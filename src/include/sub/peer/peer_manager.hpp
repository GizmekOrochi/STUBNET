#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "peer.hpp"

namespace Stubnet {

class PeerManager {

public:
    void addPeer(peer value);
    void removePeer(const std::string& id);

    std::vector<peer> getPeers() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, peer> peers_;
};

}
