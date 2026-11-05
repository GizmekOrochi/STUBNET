#include "../../include/sub/discovery.hpp"

#include <charconv>
#include <cctype>
#include <limits>
#include <sstream>
#include <string_view>
#include <vector>

namespace Stubnet {
namespace {
bool validText(const std::string& value) {
    return !value.empty() && value.size() <= 128 &&
           value.find_first_of("|\r\n") == std::string::npos;
}

bool validIdentity(const std::string& id) {
    if (id.size() != 64) return false;
    for (unsigned char c : id) {
        if (!std::isxdigit(c)) return false;
    }
    return true;
}

template <typename T>
bool parseNumber(const std::string& text, T& result) {
    if (text.empty()) return false;
    unsigned long long parsed = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (ec != std::errc{} || end != text.data() + text.size() ||
        parsed > std::numeric_limits<T>::max()) return false;
    result = static_cast<T>(parsed);
    return true;
}
} // namespace

std::string Discovery::announcement(const std::string& project, const std::string& name,
    std::uint16_t port, std::uint32_t version, const std::string& identity) {
    return "STUBNET1|" + project + "|" + name + "|" + std::to_string(port) +
           "|" + std::to_string(version) + "|" + identity;
}

bool Discovery::parse(const std::string& text, std::string& project, std::string& name,
    std::uint16_t& port, std::uint32_t& version, std::string& identity) {
    std::vector<std::string> fields;
    std::istringstream in(text);
    std::string token;
    while (std::getline(in, token, '|')) fields.push_back(token);
    if (fields.size() != 6 || fields[0] != "STUBNET1" ||
        !validText(fields[1]) || !validText(fields[2]) || !validIdentity(fields[5]))
        return false;

    std::uint16_t parsedPort{};
    std::uint32_t parsedVersion{};
    if (!parseNumber(fields[3], parsedPort) || parsedPort == 0 ||
        !parseNumber(fields[4], parsedVersion)) return false;

    project = fields[1];
    name = fields[2];
    port = parsedPort;
    version = parsedVersion;
    identity = fields[5];
    return true;
}
} // namespace Stubnet
