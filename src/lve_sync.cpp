#include "lve_sync.hpp"

#include <iostream>

namespace lve {



LveSync::LveSync() {
    stopRendering = false;
}

LveSync* LveSync::getInstance() {
    if (instance == nullptr) {
        instance = std::unique_ptr<LveSync>(new LveSync());
    }
    return instance.get();
}

LveSync::~LveSync() {
}

}  // namespace lve