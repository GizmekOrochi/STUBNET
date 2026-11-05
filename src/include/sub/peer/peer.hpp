#pragma once

#include <cstdint>
#include <string>

namespace Stubnet {

enum class PeerState {
    Connected,
    Disconnected
};

struct peer {
    std::string id; // SHA-256 fingerprint of the peer certificate
    std::string name;
    std::string ip;
    std::uint16_t port = 0;
    std::uint32_t version = 0;
    PeerState state = PeerState::Disconnected;
};

}
