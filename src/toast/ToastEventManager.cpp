#include "../../include/lunalify/ToastEventManager.hpp"
#include <condition_variable>

std::queue<ToastEvent> ToastEventManager::_eventQueue;
std::mutex ToastEventManager::_queueMutex;
std::condition_variable ToastEventManager::_cv;

void ToastEventManager::PushEvent(const std::string& id, const std::string& name, const std::string& args) {
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _eventQueue.push({ id, name, args });
    }
    // Notify one waiting thread that an event has been pushed
    _cv.notify_one();
}

bool ToastEventManager::PopEvent(ToastEvent& outEvent) {
    std::unique_lock<std::mutex> lock(_queueMutex);

    // Locks the thread until the queue is not empty
    _cv.wait(lock, [] { return !_eventQueue.empty(); });

    outEvent = _eventQueue.front();
    _eventQueue.pop();
    return true;
}

bool ToastEventManager::HasEvents() {
    std::lock_guard<std::mutex> lock(_queueMutex);
    return !_eventQueue.empty();
}
