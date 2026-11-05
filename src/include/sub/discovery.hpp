#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <string>

namespace Stubnet {

class Discovery {
public:
    static constexpr unsigned short Port{42001};
    static constexpr const char* Group{"239.255.42.99"};
    // Announcements contain hints only; they are NOT authenticated.
    static std::string announcement(const std::string& project, const std::string& name, std::uint16_t port, std::uint32_t version, const std::string& identity);
    static bool parse(const std::string& text, std::string& project, std::string& name, std::uint16_t& port, std::uint32_t& version, std::string& identity);
};

}
