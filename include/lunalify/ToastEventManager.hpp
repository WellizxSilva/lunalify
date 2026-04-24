#pragma once
#include <mutex>
#include <string>
#include <queue>
#include <condition_variable>

struct ToastEvent {
    std::string toastId;
    std::string eventName;
    std::string args;
};

class ToastEventManager {

public:
    static void PushEvent(const std::string& id, const std::string& name, const std::string& args);

    static bool PopEvent(ToastEvent& outEvent);
    static bool HasEvents();

private:
    static std::queue<ToastEvent> _eventQueue;
    static std::mutex _queueMutex;
    static std::condition_variable _cv;
};
