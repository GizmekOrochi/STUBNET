#include "../../../include/sub/peer/peer_manager.hpp"
namespace Stubnet {
void PeerManager::addPeer(peer value) {
    std::lock_guard lock(mutex_);
    peers_[value.id] = std::move(value);
}
void PeerManager::removePeer(const std::string& id) {
    std::lock_guard lock(mutex_);
    peers_.erase(id);
}
std::vector<peer> PeerManager::getPeers() const {
    std::lock_guard lock(mutex_);
    std::vector<peer> result;
    for (const auto& [id, value] : peers_) result.push_back(value);
    return result;
}
}
