#include "../../include/sub/network.hpp"
#include "../../include/sub/discovery.hpp"
#include "../../include/sub/event/eventbus.hpp"
#include "../../include/sub/peer/peer_manager.hpp"
#include "../../include/sub/communication/security.hpp"
#include "../../include/sub/communication/transport.hpp"
#include <boost/asio.hpp>
#include <atomic>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>

namespace Stubnet {
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using udp = asio::ip::udp;
using namespace std::chrono_literals;

namespace {
bool validField(const std::string& text) {
    return !text.empty() && text.size() <= 128 &&
           text.find_first_of("|\r\n") == std::string::npos;
}

bool validIdentity(const std::string& id) {
    if (id.size() != 64) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

template <typename T>
bool parseNumber(const std::string& text, T& result) {
    if (text.empty()) return false;
    unsigned long long parsed = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (ec != std::errc{} || end != text.data() + text.size() ||
        parsed > std::numeric_limits<T>::max()) return false;
    result = static_cast<T>(parsed);
    return true;
}

bool parseHello(const std::string& data, std::string& project, std::string& name,
                std::uint32_t& version, std::uint16_t& port, std::string& id) {
    // H|project|name|version|listening-port|certificate-fingerprint
    if (data.rfind("H|", 0) != 0) return false;
    auto a = data.find('|', 2);
    auto b = a == std::string::npos ? a : data.find('|', a + 1);
    auto c = b == std::string::npos ? b : data.find('|', b + 1);
    auto d = c == std::string::npos ? c : data.find('|', c + 1);
    if (a == std::string::npos || b == std::string::npos ||
        c == std::string::npos || d == std::string::npos) return false;
    project = data.substr(2, a - 2);
    name = data.substr(a + 1, b - a - 1);
    id = data.substr(d + 1);
    return validField(project) && validField(name) && validIdentity(id) &&
           parseNumber(data.substr(b + 1, c - b - 1), version) &&
           parseNumber(data.substr(c + 1, d - c - 1), port) && port != 0;
}
} // namespace

struct Network::Impl {
    struct Session {
        Transport::Socket stream;
        std::mutex sendMutex;
        peer remote;
        bool outgoing = false;
        explicit Session(asio::io_context& io, asio::ssl::context& ssl, bool isOutgoing)
            : stream(io, ssl), outgoing(isOutgoing) {}
        void close() {
            boost::system::error_code ec;
            stream.next_layer().cancel(ec);
            stream.next_layer().shutdown(tcp::socket::shutdown_both, ec);
            stream.next_layer().close(ec);
        }
        bool send(const std::string& data) {
            std::lock_guard lock(sendMutex);
            return Transport::send(stream, data);
        }
    };

    config conf;
    Security security;
    asio::io_context io;
    tcp::acceptor acceptor{io};
    udp::socket discoverySocket{io};
    udp::socket announceSocket{io};
    EventBus events;
    PeerManager peers;
    Network::PeerCallback onRequestCallback;
    Network::PeerCallback onRequestedCallback;
    std::mutex callbackMutex;
    std::mutex sessionsMutex;
    std::vector<std::shared_ptr<Session>> allSessions;
    std::map<std::string, std::shared_ptr<Session>> connected;
    std::set<std::string> connecting;
    struct Worker {
        std::thread thread;
        std::shared_ptr<std::atomic<bool>> finished;
    };
    std::vector<Worker> workers;
    static constexpr std::size_t MaxConcurrentSessions = 128;
    std::thread acceptThread, receiveThread, announceThread;
    std::atomic<bool> running{false};
    std::atomic<std::uint32_t> version;

    explicit Impl(config c) : conf(std::move(c)), security(conf), version(conf.version) {
        if (!validField(conf.project) || !validField(conf.name) || !conf.port ||
            conf.discovery_interval < 250)
            throw std::invalid_argument("Invalid name, project, port or discovery_interval");
    }
    ~Impl() { stop(); }

    void start() {
        if (running.exchange(true)) return;
        try {
            tcp::endpoint endpoint(tcp::v4(), conf.port);
            acceptor.open(endpoint.protocol());
            acceptor.set_option(tcp::acceptor::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen();
            acceptor.non_blocking(true);

            udp::endpoint listen(udp::v4(), Discovery::Port);
            discoverySocket.open(udp::v4());
            discoverySocket.set_option(udp::socket::reuse_address(true));
            discoverySocket.bind(listen);
            discoverySocket.set_option(asio::ip::multicast::join_group(
                asio::ip::make_address_v4(Discovery::Group)));
            discoverySocket.non_blocking(true);

            announceSocket.open(udp::v4());
            announceSocket.set_option(asio::ip::multicast::enable_loopback(true));
            announceSocket.set_option(asio::ip::multicast::hops(1));
            acceptThread = std::thread([this]{ acceptLoop(); });
            receiveThread = std::thread([this]{ receiveLoop(); });
            announceThread = std::thread([this]{ announceLoop(); });
        } catch (...) { stop(); throw; }
    }
    void stop() {
        if (!running.exchange(false)) return;
        boost::system::error_code ec;
        acceptor.close(ec);
        discoverySocket.close(ec);
        announceSocket.close(ec);
        if (acceptThread.joinable()) acceptThread.join();
        if (receiveThread.joinable()) receiveThread.join();
        if (announceThread.joinable()) announceThread.join();
        {
            std::lock_guard lock(sessionsMutex);
            for (auto& session : allSessions) session->close();
        }
        // No more threads are spawned after accept/receive have exited.
        for (auto& worker : workers) if (worker.thread.joinable()) worker.thread.join();
        workers.clear();
        std::lock_guard lock(sessionsMutex);
        allSessions.clear();
        connected.clear();
        connecting.clear();
        for (const auto& p : peers.getPeers()) peers.removePeer(p.id);
    }
    void spawn(std::shared_ptr<Session> session, std::string expectedId = {}) {
        std::lock_guard lock(sessionsMutex);
        if (!running) return;
        // Reap finished workers: repeated reconnects must not leak thread handles.
        for (auto it = workers.begin(); it != workers.end();) {
            if (it->finished->load()) {
                if (it->thread.joinable()) it->thread.join();
                it = workers.erase(it);
            } else {
                ++it;
            }
        }
        if (allSessions.size() >= MaxConcurrentSessions) {
            if (!expectedId.empty()) {
                const auto at = expectedId.find('@');
                connecting.erase(expectedId.substr(0, at));
            }
            session->close();
            return;
        }
        allSessions.push_back(session);
        auto finished = std::make_shared<std::atomic<bool>>(false);
        try {
            workers.push_back({std::thread([this, session, finished,
                                            expectedId = std::move(expectedId)] {
                runSession(session, expectedId);
                finished->store(true);
            }), finished});
        } catch (...) {
            allSessions.pop_back();
            session->close();
            throw;
        }
    }
    void acceptLoop() {
        while (running) {
            auto session = std::make_shared<Session>(io, security.context(), false);
            boost::system::error_code ec;
            acceptor.accept(session->stream.next_layer(), ec);
            if (!ec) spawn(std::move(session));
            else if (ec != asio::error::would_block && ec != asio::error::try_again && running)
                std::cerr << "Stubnet accept: " << ec.message() << '\n';
            std::this_thread::sleep_for(40ms);
        }
    }
    void announceLoop() {
        const udp::endpoint destination(asio::ip::make_address_v4(Discovery::Group), Discovery::Port);
        while (running) {
            std::string announcement = Discovery::announcement(conf.project, conf.name, conf.port,
                version.load(), security.identity());
            boost::system::error_code ec;
            announceSocket.send_to(asio::buffer(announcement), destination, 0, ec);
            auto waited = 0u;
            while (running && waited < conf.discovery_interval) {
                std::this_thread::sleep_for(50ms);
                waited += 50;
            }
        }
    }
    void receiveLoop() {
        std::array<char, 2048> buffer{};
        while (running) {
            udp::endpoint sender;
            boost::system::error_code ec;
            std::size_t n = discoverySocket.receive_from(asio::buffer(buffer), sender, 0, ec);
            if (ec) { std::this_thread::sleep_for(40ms); continue; }
            std::string project, name, id;
            std::uint16_t port = 0;
            std::uint32_t remoteVersion = 0;
            if (!Discovery::parse(std::string(buffer.data(), n), project, name, port, remoteVersion, id)) continue;
            if (project != conf.project || id == security.identity()) continue;
            // Deterministic initiator avoids duplicate connections between peers.
            if (security.identity() > id) continue;
            {
                std::lock_guard lock(sessionsMutex);
                if (connected.contains(id) || connecting.contains(id)) continue;
                connecting.insert(id);
            }
            auto session = std::make_shared<Session>(io, security.context(), true);
            spawn(std::move(session), id + "@" + sender.address().to_string() + ":" + std::to_string(port));
        }
    }
    void runSession(const std::shared_ptr<Session>& session, const std::string& expected) {
        std::string expectedId, address;
        std::uint16_t remotePort = 0;
        if (session->outgoing) {
            auto at = expected.find('@'), colon = expected.rfind(':');
            if (at == std::string::npos || colon == std::string::npos) return;
            expectedId = expected.substr(0, at);
            address = expected.substr(at+1, colon-at-1);
            try { remotePort = static_cast<std::uint16_t>(std::stoul(expected.substr(colon+1))); }
            catch (...) { return; }
        }
        auto cleanup = [&] {
            session->close();
            std::lock_guard lock(sessionsMutex);
            if (!expectedId.empty()) connecting.erase(expectedId);
            if (!session->remote.id.empty()) {
                auto it = connected.find(session->remote.id);
                if (it != connected.end() && it->second == session) {
                    connected.erase(it);
                    peers.removePeer(session->remote.id);
                }
            }
            std::erase(allSessions, session);
        };
        try {
            if (session->outgoing) {
                session->stream.next_layer().connect(tcp::endpoint(asio::ip::make_address(address), remotePort));
            }
            session->stream.handshake(session->outgoing ? asio::ssl::stream_base::client :
                                                          asio::ssl::stream_base::server);
            if (SSL_get_verify_result(session->stream.native_handle()) != X509_V_OK)
                throw std::runtime_error("Certificate not trusted");
            X509* cert = SSL_get1_peer_certificate(session->stream.native_handle());
            if (!cert) throw std::runtime_error("Missing peer certificate");
            std::string certId;
            try { certId = Security::fingerprint(cert); }
            catch (...) { X509_free(cert); throw; }
            X509_free(cert);
            if (!expectedId.empty() && expectedId != certId)
                throw std::runtime_error("Discovery identity does not match TLS certificate");
            std::string localHello = "H|" + conf.project + "|" + conf.name + "|" +
                std::to_string(version.load()) + "|" + std::to_string(conf.port) +
                "|" + security.identity();
            if (!session->send(localHello)) throw std::runtime_error("Cannot send hello");
            std::string data, project, name, id;
            std::uint32_t remoteVersion = 0;
            std::uint16_t listeningPort = 0;
            if (!Transport::receive(session->stream, data) ||
                !parseHello(data, project, name, remoteVersion, listeningPort, id) ||
                project != conf.project || id != certId)
                throw std::runtime_error("Invalid authenticated hello");
            auto ep = session->stream.next_layer().remote_endpoint();
            session->remote = {id, name, ep.address().to_string(), listeningPort, remoteVersion, PeerState::Connected};
            {
                std::lock_guard lock(sessionsMutex);
                if (!running || connected.contains(id)) { cleanupWithoutLock(session, expectedId); return; }
                connected[id] = session;
                connecting.erase(id);
                peers.addPeer(session->remote);
            }
            Network::PeerCallback cb;
            {
                std::lock_guard lock(callbackMutex);
                cb = session->outgoing ? onRequestCallback : onRequestedCallback;
            }
            try { if (cb) cb(session->remote); } catch (const std::exception& e) {
                std::cerr << "Stubnet peer callback: " << e.what() << '\n';
            } catch (...) {
                std::cerr << "Stubnet peer callback: unknown exception\n";
            }
            while (running && Transport::receive(session->stream, data)) {
                if (data.rfind("M|", 0) != 0) continue;
                auto separator = data.find('|', 2);
                if (separator == std::string::npos) continue;
                message msg{data.substr(2, separator-2), data.substr(separator+1), session->remote};
                try { events.dispatch(msg); }
                catch (const std::exception& e) { std::cerr << "Stubnet message callback: " << e.what() << '\n'; }
                catch (...) { std::cerr << "Stubnet message callback: unknown exception\n"; }
            }
        } catch (const std::exception& e) {
            if (running) std::cerr << "Stubnet session: " << e.what() << '\n';
        }
        cleanup();
    }
    // Caller already holds sessionsMutex.
    void cleanupWithoutLock(const std::shared_ptr<Session>& session, const std::string& expectedId) {
        if (!expectedId.empty()) connecting.erase(expectedId);
        session->close();
        std::erase(allSessions, session);
    }
    void broadcast(const message& msg) {
        if (!validField(msg.header)) throw std::invalid_argument("Invalid message header");
        std::string data = "M|" + msg.header + "|" + msg.body;
        if (data.size() > 64 * 1024) throw std::length_error("Message too large");
        std::vector<std::shared_ptr<Session>> targets;
        {
            std::lock_guard lock(sessionsMutex);
            for (auto& [id, session] : connected) targets.push_back(session);
        }
        for (auto& session : targets) {
            if (!session->send(data)) session->close();
        }
    }
};

Network::Network(config conf) : impl_(std::make_unique<Impl>(std::move(conf))) {}
Network::~Network() = default;
void Network::onRequest(PeerCallback callback) {
    std::lock_guard lock(impl_->callbackMutex);
    impl_->onRequestCallback = std::move(callback);
}
void Network::onRequested(PeerCallback callback) {
    std::lock_guard lock(impl_->callbackMutex);
    impl_->onRequestedCallback = std::move(callback);
}
void Network::on(const std::string& header, MessageCallback callback) {
    if (!validField(header)) throw std::invalid_argument("Invalid event header");
    impl_->events.subscribe(header, std::move(callback));
}
void Network::start() { impl_->start(); }
void Network::stop() { impl_->stop(); }
void Network::broadcast(const message& msg) { impl_->broadcast(msg); }
void Network::setVersion(std::uint32_t v) { impl_->version.store(v); }
std::vector<peer> Network::getPeers() const { return impl_->peers.getPeers(); }
} // namespace Stubnet
