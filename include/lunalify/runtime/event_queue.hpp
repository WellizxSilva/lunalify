#pragma once
#include <mutex>
#include <string>
#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Lunalify::Runtime {
    struct ToastEvent {
        std::string toastId;
        std::string eventName;
        std::string args;
    };

    class EventQueue {
    public:
        EventQueue() = default;
        ~EventQueue() = default;
        EventQueue(const EventQueue&) = delete;
        EventQueue& operator=(const EventQueue&) = delete;
        void Push(std::string id, std::string name, std::string args);
        // Blocks until an event is available, then pops it from the queue.
        ToastEvent WaitAndPop();
        std::optional<ToastEvent> TryPop();
        bool IsEmpty() const;
    private:
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<ToastEvent> m_queue;
    };
}
