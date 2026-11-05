#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>

namespace Stubnet {

class Transport {
public:
    using Socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    
    static bool send(Socket& socket, const std::string& payload);
    static bool receive(Socket& socket, std::string& payload);
};

}
