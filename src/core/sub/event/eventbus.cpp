#include "../../../include/sub/event/eventbus.hpp"
namespace Stubnet {
void EventBus::subscribe(const std::string& header, Callback callback) {
    std::lock_guard lock(mutex_);
    callbacks_[header] = std::move(callback);
}
void EventBus::dispatch(const message& msg) const {
    Callback callback;
    {
        std::lock_guard lock(mutex_);
        auto it = callbacks_.find(msg.header);
        if (it != callbacks_.end()) callback = it->second;
    }
    if (callback) callback(msg); // never invoke user code under lock
}
}
