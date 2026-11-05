
#include <include/stubnet.hpp>
#include <iostream>


void onRequest(const Stubnet::peer& p) {
    std::cout << "Connexion vers " << p.name;
}

void onRequested(const Stubnet::peer& p) {
    std::cout << "Connexion de " << p.name;
}

void onBuildReady(const Stubnet::message& m) {
    std::cout << m.body << "\n";
}

int main() {
    Stubnet::config conf{
        .name = "LECACARPOUTE",
        .version = 1,
        .port = 42000,
        .discovery_interval = 5000,
        .security = Stubnet::security::TLS13
    };

    Stubnet::Network net(conf);

    // Aucun [] !
    net.onRequest(onRequest);
    net.onRequested(onRequested);
    net.on("build-ready", onBuildReady);

    net.start();

    net.broadcast({
        .header = "build-ready",
        .body = "LE CACAAAA"
    });

    runApplication();
}
