#pragma once

#include <boost/asio/ssl.hpp>
#include <string>

#include "../config.hpp"

namespace Stubnet {

class Security {
public:
    explicit Security(const config& conf);
    boost::asio::ssl::context& context() { return ctx_; }

    const std::string& identity() const { return identity_; }
    static std::string fingerprint(X509* certificate);

private:
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tls};
    std::string identity_;
};

}
