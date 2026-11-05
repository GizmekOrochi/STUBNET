#pragma once

#include <cstdint>
#include <string>

namespace Stubnet {

enum class security {
    TLS13
};

struct config {
    std::string name{"StubnetNode"};
    std::uint32_t version{1};
    std::uint16_t port{42000};
    std::uint32_t discovery_interval{5000}; // milliseconds
    Stubnet::security security{Stubnet::security::TLS13};
    std::string project{"default"};
    std::string certificate{}; // PEM, signed by project CA
    std::string private_key{}; // PEM
    std::string ca_certificate{}; // PEM trust anchor
};

} // namespace Stubnet
