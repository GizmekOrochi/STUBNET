#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "config.hpp"
#include "peer/peer.hpp"
#include "event/message.hpp"

namespace Stubnet {

class Network {
public:
    using PeerCallback = std::function<void(const peer&)>;
    using MessageCallback = std::function<void(const message&)>;
    
    explicit Network(config conf);
    ~Network();
    Network(const Network&) = delete;

    Network& operator=(const Network&) = delete;

    void onRequest(PeerCallback callback);
    void onRequested(PeerCallback callback);
    void on(const std::string& header, MessageCallback callback);
    void start();
    void stop();
    void broadcast(const message& msg);
    void setVersion(std::uint32_t version);

    std::vector<peer> getPeers() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
