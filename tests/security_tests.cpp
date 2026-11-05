#include "tester.hpp"
#include <sub/communication/security.hpp>

#include <stdexcept>
#include <string>

namespace {
    
TEST(Security, MissingCertificateRejected) {
    Stubnet::config conf;
    bool rejected{};
    try { Stubnet::Security security(conf); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

TEST(Security, NullCertificateFingerprintRejected) {
    bool rejected{};
    try { (void)Stubnet::Security::fingerprint(nullptr); }
    catch (const std::runtime_error&) { rejected = true; }
    CHECK(rejected);
}

}
