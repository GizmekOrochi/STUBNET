#include "../../../include/sub/communication/transport.hpp"
#include <array>
#include <cstdint>
namespace Stubnet {
namespace asio = boost::asio;
static constexpr std::size_t MaxFrame = 64 * 1024;
bool Transport::send(Socket& socket, const std::string& payload) {
    if (payload.size() > MaxFrame) return false;
    const std::uint32_t n = static_cast<std::uint32_t>(payload.size());
    std::array<unsigned char,4> prefix{
        static_cast<unsigned char>(n >> 24), static_cast<unsigned char>(n >> 16),
        static_cast<unsigned char>(n >> 8), static_cast<unsigned char>(n)};
    boost::system::error_code ec;
    asio::write(socket, asio::buffer(prefix), ec);
    if (ec) return false;
    asio::write(socket, asio::buffer(payload), ec);
    return !ec;
}
bool Transport::receive(Socket& socket, std::string& payload) {
    std::array<unsigned char,4> prefix{};
    boost::system::error_code ec;
    asio::read(socket, asio::buffer(prefix), ec);
    if (ec) return false;
    const std::uint32_t n = (std::uint32_t(prefix[0]) << 24) |
        (std::uint32_t(prefix[1]) << 16) | (std::uint32_t(prefix[2]) << 8) | prefix[3];
    if (n > MaxFrame) return false;
    payload.resize(n);
    if (n) asio::read(socket, asio::buffer(payload.data(), n), ec);
    return !ec;
}
}
