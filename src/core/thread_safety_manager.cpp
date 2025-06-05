// ---------------------------------------------
// Thread Safety Manager Implementation
// Centralizes all thread synchronization for the Listeningway system
// ---------------------------------------------
#include "thread_safety_manager.h"
#include <algorithm>
#include <vector>

namespace Listeningway {

ThreadSafetyManager& ThreadSafetyManager::Instance() {
    static ThreadSafetyManager instance;
    return instance;
}

std::mutex& ThreadSafetyManager::GetMutex(LockType type) {
    switch (type) {
        case LockType::LOGGING:
            return logging_mutex_;
        case LockType::AUDIO_DATA:
            return audio_data_mutex_;
        case LockType::PROVIDER_SWITCH:
            return provider_switch_mutex_;
        default:
            return audio_data_mutex_; // Safe fallback
    }
}

// SingleLock implementation
ThreadSafetyManager::SingleLock::SingleLock(LockType type) 
    : lock_(ThreadSafetyManager::Instance().GetMutex(type)), type_(type) {
}

ThreadSafetyManager::SingleLock::~SingleLock() = default;

// MultipleLock implementation
ThreadSafetyManager::MultipleLock::MultipleLock(std::initializer_list<LockType> types) {
    // Convert to vector and sort by lock priority to prevent deadlock
    std::vector<LockType> sorted_types(types);
    std::sort(sorted_types.begin(), sorted_types.end());
    
    // Lock mutexes in priority order
    locks_.reserve(sorted_types.size());
    for (LockType type : sorted_types) {
        locks_.emplace_back(ThreadSafetyManager::Instance().GetMutex(type));
    }
}

ThreadSafetyManager::MultipleLock::~MultipleLock() = default;

} // namespace Listeningway
