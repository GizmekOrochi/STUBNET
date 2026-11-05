#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include <stubnet/stubnet.hpp>
#include <sub/discovery.hpp>
#include <sub/event/eventbus.hpp>
#include <sub/peer/peer_manager.hpp>

namespace {
    
void require(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
}

void testDiscovery() {
    std::string project, name, id;
    std::uint16_t port{};
    std::uint32_t version{};
    const std::string fingerprint(64, 'a');

    const auto wire{Stubnet::Discovery::announcement("project", "node", 42000, 7, fingerprint)};
    require(Stubnet::Discovery::parse(wire, project, name, port, version, id), "valid announcement");
    require(project == "project" && name == "node" && port == 42000 && version == 7,
            "parsed announcement fields");
    require(id == fingerprint, "parsed certificate identity");

    const auto maxVersion{Stubnet::Discovery::announcement("project", "node", 65535, UINT32_MAX, fingerprint)};
    require(Stubnet::Discovery::parse(maxVersion, project, name, port, version, id), "maximum numeric values");
    require(port == 65535 && version == UINT32_MAX, "maximum values preserved");

    const std::string invalid[] = {
        "garbage",
        "STUBNET1|project|node|0|1|" + fingerprint,
        "STUBNET1|project|node|65536|1|" + fingerprint,
        "STUBNET1|project|node|42000oops|1|" + fingerprint,
        "STUBNET1|project|node|42000|-1|" + fingerprint,
        "STUBNET1|project|node|42000|4294967296|" + fingerprint,
        "STUBNET1|project|node|42000|1|" + std::string(64, 'z'),
        "STUBNET1|project|node|42000|1|" + fingerprint + "|extra",
        "STUBNET1||node|42000|1|" + fingerprint,
        "STUBNET1|project|node|42000|1|" + fingerprint + "garbage"
    };
    for (const auto& data : invalid) {
        require(!Stubnet::Discovery::parse(data, project, name, port, version, id),
                "malformed announcement rejected");
    }
}

void testPeerManager() {
    Stubnet::PeerManager manager;
    require(manager.getPeers().empty(), "empty peer registry");
    Stubnet::peer alpha{"id-1", "alpha", "127.0.0.1", 42000, 1, Stubnet::PeerState::Connected};
    manager.addPeer(alpha);
    require(manager.getPeers().size() == 1, "peer inserted");
    alpha.version = 2;
    manager.addPeer(alpha);

    const auto peers{manager.getPeers()};
    require(peers.size() == 1 && peers[0].version == 2, "peer updated without duplication");
    manager.removePeer(alpha.id);
    require(manager.getPeers().empty(), "peer removed");
}

void testEventBus() {
    Stubnet::EventBus events;
    int calls{};
    events.subscribe("build-ready", [&calls](const Stubnet::message& msg) {
        require(msg.header == "build-ready" && msg.body == "hello", "event payload");
        ++calls;
    });

    events.dispatch({.header = "ignored", .body = "hello"});
    require(calls == 0, "unknown event ignored");
    events.dispatch({.header = "build-ready", .body = "hello"});
    require(calls == 1, "event delivered once");
    events.subscribe("build-ready", [&calls](const Stubnet::message&) { calls += 10; });
    events.dispatch({.header = "build-ready", .body = "hello"});
    require(calls == 11, "handler replacement");
}

} // namespace

int main() {
    try {
        testDiscovery();
        testPeerManager();
        testEventBus();
        std::cout << "Core tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Core test failure: " << e.what() << '\n';
        return 1;
    }
}
