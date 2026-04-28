#include "lunalify/runtime/event_queue.hpp"


namespace Lunalify::Runtime {
    void EventQueue::Push(std::string id, std::string name, std::string args) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // std::move to avoid unnecessary copies
            m_queue.push({ std::move(id), std::move(name), std::move(args) });
        }
        // notify one waiting thread that an event is available
        m_cv.notify_one();
    }
    ToastEvent EventQueue::WaitAndPop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        // wait until an event is available
        m_cv.wait(lock, [this]() { return !m_queue.empty(); });

        ToastEvent event = std::move(m_queue.front());
        m_queue.pop();
        return event;
    }
    std::optional<ToastEvent> EventQueue::TryPop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return std::nullopt; // Returns immediately without blocking
        }

        ToastEvent event = std::move(m_queue.front());
        m_queue.pop();
        return event;
    }
    bool EventQueue::IsEmpty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
}
