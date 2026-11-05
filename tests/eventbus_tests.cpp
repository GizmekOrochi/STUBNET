#include "tester.hpp"
#include <sub/event/eventbus.hpp>

#include <atomic>
#include <thread>
#include <vector>

namespace {

int received{};

void countEvent(const Stubnet::message& msg) {
    CHECK_EQ(msg.header, "build-ready");
    CHECK_EQ(msg.body, "hello");
    ++received;
}

void countTen(const Stubnet::message&) { received += 10; }

TEST(EventBus, UnknownEventIgnored) {
    Stubnet::EventBus events;
    received = 0;
    events.subscribe("build-ready", countEvent);
    events.dispatch({.header="unknown", .body="hello"});
    CHECK_EQ(received, 0);
}

TEST(EventBus, DispatchAndReplacement) {
    Stubnet::EventBus events;
    received = 0;
    events.subscribe("build-ready", countEvent);
    events.dispatch({.header="build-ready", .body="hello"});
    CHECK_EQ(received, 1);
    events.subscribe("build-ready", countTen);
    events.dispatch({.header="build-ready", .body="hello"});
    CHECK_EQ(received, 11);
}

Stubnet::EventBus* reentrantBus{};
int reentrantCalls{};

void secondHandler(const Stubnet::message&) { ++reentrantCalls; }

void reentrantHandler(const Stubnet::message&) {
    ++reentrantCalls;
    reentrantBus->subscribe("event", secondHandler);
}

TEST(EventBus, HandlerCanResubscribeWithoutDeadlock) {
    Stubnet::EventBus events;
    reentrantBus = &events;
    reentrantCalls = 0;
    events.subscribe("event", reentrantHandler);
    events.dispatch({.header="event"});
    events.dispatch({.header="event"});
    CHECK_EQ(reentrantCalls, 2);
    reentrantBus = nullptr;
}

std::atomic<int> atomicCalls{0};

void atomicHandler(const Stubnet::message&) { atomicCalls.fetch_add(1, std::memory_order_relaxed); }

void dispatchMany(const Stubnet::EventBus* events) {
    for (int i{}; i < 250; ++i) events->dispatch({.header="event"});
}

TEST(EventBus, ConcurrentDispatch) {
    Stubnet::EventBus events;
    atomicCalls = 0;
    events.subscribe("event", atomicHandler);
    std::vector<std::thread> threads;
    for (int i{}; i < 4; ++i) threads.emplace_back(dispatchMany, &events);
    for (auto& thread : threads) thread.join();
    CHECK_EQ(atomicCalls.load(), 1000);
}

TEST(EventBus, EmptyCallbackIsSafe) {
    Stubnet::EventBus events;
    events.subscribe("event", {});
    events.dispatch({.header="event"});
    CHECK(true);
}

}
