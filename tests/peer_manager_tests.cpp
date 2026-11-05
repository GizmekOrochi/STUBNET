#include "tester.hpp"
#include <sub/peer/peer_manager.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace {
    
TEST(PeerManager, StartsEmpty) {
    Stubnet::PeerManager peers;
    CHECK(peers.getPeers().empty());
}

TEST(PeerManager, InsertUpdateRemove) {
    Stubnet::PeerManager peers;
    Stubnet::peer alpha{"id-1", "alpha", "127.0.0.1", 42000, 1, Stubnet::PeerState::Connected};
    peers.addPeer(alpha);
    CHECK_EQ(peers.getPeers().size(), 1u);
    alpha.version = 2;
    peers.addPeer(alpha);
    const auto snapshot = peers.getPeers();
    CHECK_EQ(snapshot.size(), 1u);
    CHECK_EQ(snapshot.front().version, 2u);
    peers.removePeer("id-1");
    peers.removePeer("id-1");
    CHECK(peers.getPeers().empty());
}

TEST(PeerManager, SnapshotsAreIndependent) {
    Stubnet::PeerManager peers;
    peers.addPeer({"id-1", "alpha", "127.0.0.1", 42000, 1, Stubnet::PeerState::Connected});
    auto snapshot{peers.getPeers()};
    snapshot.front().name = "modified";
    const auto fresh{peers.getPeers()};
    CHECK_EQ(fresh.front().name, "alpha");
}

void addRange(Stubnet::PeerManager* peers, int first, int count) {
    for (int i{first}; i < first + count; ++i)
        peers->addPeer({std::to_string(i), "node", "127.0.0.1", 42000, 1, Stubnet::PeerState::Connected});
}

TEST(PeerManager, ConcurrentInsertions) {
    Stubnet::PeerManager peers;
    std::vector<std::thread> threads;
    for (int i{}; i < 4; ++i)
        threads.emplace_back(addRange, &peers, i * 250, 250);
    for (auto& thread : threads) thread.join();
    CHECK_EQ(peers.getPeers().size(), 1000u);
}

}
