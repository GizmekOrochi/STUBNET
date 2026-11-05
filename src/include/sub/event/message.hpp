#pragma once

#include <string>

#include "../peer/peer.hpp"

namespace Stubnet {

struct message {
    std::string header;
    std::string body;
    peer sender; // populated on receive from authenticated connection
};

}
