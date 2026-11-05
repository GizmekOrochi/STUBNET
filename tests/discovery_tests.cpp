#include "tester.hpp"
#include <sub/discovery.hpp>

#include <cstdint>
#include <limits>
#include <string>

namespace {
constexpr const char* Fingerprint = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

struct Parsed {
    std::string project, name, id;
    std::uint16_t port{};
    std::uint32_t version{};
    bool parse(const std::string& input) {
        return Stubnet::Discovery::parse(input, project, name, port, version, id);
    }
};

TEST(Discovery, RoundTrip) {
    Parsed p;
    CHECK(p.parse(Stubnet::Discovery::announcement("my-project", "node", 42000, 7, Fingerprint)));
    CHECK_EQ(p.project, "my-project");
    CHECK_EQ(p.name, "node");
    CHECK_EQ(p.port, 42000);
    CHECK_EQ(p.version, 7u);
    CHECK_EQ(p.id, Fingerprint);
}

TEST(Discovery, NumericBoundaries) {
    Parsed p;
    CHECK(p.parse(Stubnet::Discovery::announcement("p", "n", 1, 0, Fingerprint)));
    CHECK_EQ(p.port, 1);
    CHECK_EQ(p.version, 0u);
    CHECK(p.parse(Stubnet::Discovery::announcement("p", "n", 65535,
          std::numeric_limits<std::uint32_t>::max(), Fingerprint)));
    CHECK_EQ(p.port, 65535);
    CHECK_EQ(p.version, std::numeric_limits<std::uint32_t>::max());
}

TEST(Discovery, RejectsMalformedFields) {
    const std::string bad[] = {
        "", "garbage", "STUBNET2|p|n|1|1|" + std::string(Fingerprint),
        "STUBNET1||n|1|1|" + std::string(Fingerprint),
        "STUBNET1|p||1|1|" + std::string(Fingerprint),
        "STUBNET1|p|n|0|1|" + std::string(Fingerprint),
        "STUBNET1|p|n|65536|1|" + std::string(Fingerprint),
        "STUBNET1|p|n|-1|1|" + std::string(Fingerprint),
        "STUBNET1|p|n|1|-1|" + std::string(Fingerprint),
        "STUBNET1|p|n|1|4294967296|" + std::string(Fingerprint),
        "STUBNET1|p|n|1oops|1|" + std::string(Fingerprint),
        "STUBNET1|p|n|1|1oops|" + std::string(Fingerprint),
        "STUBNET1|p|n|1|1|" + std::string(64, 'z'),
        "STUBNET1|p|n|1|1|" + std::string(63, 'a'),
        "STUBNET1|p|n|1|1|" + std::string(Fingerprint) + "|extra",
        "STUBNET1|p|n|1|1|" + std::string(Fingerprint) + "garbage",
        "STUBNET1|" + std::string(129, 'p') + "|n|1|1|" + Fingerprint,
        "STUBNET1|p|" + std::string(129, 'n') + "|1|1|" + Fingerprint,
        "STUBNET1|p\nq|n|1|1|" + std::string(Fingerprint),
        "STUBNET1|p|n\rq|1|1|" + std::string(Fingerprint)
    };
    for (const auto& value : bad) {
        Parsed p;
        CHECK(!p.parse(value));
    }
}

TEST(Discovery, RejectsTruncatedAndAppendedData) {
    const std::string valid = Stubnet::Discovery::announcement("project", "node", 42000, 3, Fingerprint);
    for (std::size_t n = 0; n < valid.size(); ++n) {
        Parsed p;
        CHECK(!p.parse(valid.substr(0, n)));
    }
    Parsed p;
    CHECK(!p.parse(valid + "|injected"));
}

TEST(Discovery, RejectsOversizedRandomLookingInput) {
    Parsed p;
    CHECK(!p.parse(std::string(100000, 'x')));
    CHECK(!p.parse("STUBNET1|p|n|1|1|" + std::string(100000, 'a')));
}

}
