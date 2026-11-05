#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "message.hpp"

namespace Stubnet {

class EventBus {
public:
    using Callback = std::function<void(const message&)>;

    void subscribe(const std::string& header, Callback callback);
    void dispatch(const message& msg) const;
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Callback> callbacks_;
};

}
