#include "tester.hpp"
#include <sub/discovery.hpp>

#include <cstdint>
#include <limits>
#include <string>

namespace {

TEST(Hardening, DiscoveryParsingNeverThrowsOnArbitraryBytes) {
    std::uint32_t state{0xC0FFEEu};
    for (int sample{}; sample < 1000; ++sample) {
        std::string input;
        const int length = sample % 255;
        for (int i{}; i < length; ++i) {
            state = state * 1664525u + 1013904223u;
            input.push_back(static_cast<char>(state >> 24));
        }
        std::string project, name, identity;
        std::uint16_t port{};
        std::uint32_t version{};
        CHECK(!Stubnet::Discovery::parse(input, project, name, port, version, identity));
    }
}

TEST(Hardening, FailedParseDoesNotCorruptOutputs) {
    std::string project{"previous"}, name{"previous"}, id{"previous"};
    std::uint16_t port{99};
    std::uint32_t version{88};
    CHECK(!Stubnet::Discovery::parse("STUBNET1|p|n|999999|1|bad", project, name, port, version, id));
    CHECK_EQ(project, "previous");
    CHECK_EQ(name, "previous");
    CHECK_EQ(port, 99);
    CHECK_EQ(version, 88u);
    CHECK_EQ(id, "previous");
}

}
