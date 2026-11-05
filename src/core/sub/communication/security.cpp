#include "../../../include/sub/communication/security.hpp"
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace Stubnet {
std::string Security::fingerprint(X509* cert) {
    if (!cert) throw std::runtime_error("Missing TLS certificate");
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    if (X509_digest(cert, EVP_sha256(), digest, &len) != 1)
        throw std::runtime_error("Cannot fingerprint certificate");
    std::ostringstream out;
    for (unsigned i = 0; i < len; ++i)
        out << std::hex << std::setfill('0') << std::setw(2) << unsigned(digest[i]);
    return out.str();
}
Security::Security(const config& conf) {
    if (conf.certificate.empty() || conf.private_key.empty() || conf.ca_certificate.empty())
        throw std::invalid_argument("TLS requires certificate, private_key and ca_certificate");
    ctx_.set_options(boost::asio::ssl::context::default_workarounds |
                     boost::asio::ssl::context::no_sslv2 |
                     boost::asio::ssl::context::no_sslv3);
    if (SSL_CTX_set_min_proto_version(ctx_.native_handle(), TLS1_3_VERSION) != 1 ||
        SSL_CTX_set_max_proto_version(ctx_.native_handle(), TLS1_3_VERSION) != 1)
        throw std::runtime_error("TLS 1.3 unavailable");
    ctx_.use_certificate_chain_file(conf.certificate);
    ctx_.use_private_key_file(conf.private_key, boost::asio::ssl::context::pem);
    ctx_.load_verify_file(conf.ca_certificate);
    ctx_.set_verify_mode(boost::asio::ssl::verify_peer |
                         boost::asio::ssl::verify_fail_if_no_peer_cert);
    FILE* file = fopen(conf.certificate.c_str(), "r");
    if (!file) throw std::runtime_error("Cannot read certificate");
    X509* cert = PEM_read_X509(file, nullptr, nullptr, nullptr);
    fclose(file);
    if (!cert) throw std::runtime_error("Invalid certificate PEM");
    try { identity_ = fingerprint(cert); }
    catch (...) { X509_free(cert); throw; }
    X509_free(cert);
}
}
