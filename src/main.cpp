#include <stubnet/stubnet.hpp>
#include <iostream>
#include <string>

void onRequest(const Stubnet::peer& p) {
    std::cout << "[outgoing] " << p.name << " v" << p.version << " connected" << std::endl;
}

void onRequested(const Stubnet::peer& p) {
    std::cout << "[incoming] " << p.name << " v" << p.version << " connected" << std::endl;
}

void onBuildReady(const Stubnet::message& m) {
    std::cout << "[build-ready] " << m.sender.name << ": " << m.body  << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <name> <port> <cert.pem> <key.pem> <ca.pem>\n";
        return 1;
    }
    try {
        const auto parsedPort = std::stoul(argv[2]);
        if (parsedPort == 0 || parsedPort > 65535)
            throw std::invalid_argument("Port must be between 1 and 65535");

        Stubnet::config conf{
            .name = argv[1],
            .version = 1,
            .port = static_cast<std::uint16_t>(parsedPort),
            .discovery_interval = 1000,
            .security = Stubnet::security::TLS13,
            .project = "stubnet-dev",
            .certificate = argv[3],
            .private_key = argv[4],
            .ca_certificate = argv[5]
        };

        Stubnet::Network net(conf);

        net.onRequest(onRequest);
        net.onRequested(onRequested);

        net.on("build-ready", onBuildReady);

        net.start();

        std::cout << "Node " << conf.name << " ready. Commands: send <text>, peers, quit"  << std::endl;
        std::string line;

        while (std::getline(std::cin, line)) {
            if (line == "quit") break;
            if (line == "peers") {
                for (const auto& p : net.getPeers())
                    std::cout << p.name << " " << p.ip << " v" << p.version  << std::endl;
            } else if (line.rfind("send ", 0) == 0) {
                net.broadcast({.header = "build-ready", .body = line.substr(5)});
            }
        }
        net.stop();
    } catch (const std::exception& e) {
        std::cerr << "Stubnet error: " << e.what() << std::endl;
        return 1;
    }
}
