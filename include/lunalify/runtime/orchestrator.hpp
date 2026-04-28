#pragma once
#include <windows.h>
#include <atomic>
#include <thread>
#include "event_queue.hpp"
#include "pipe_server.hpp"
#include "../protocol.hpp"

namespace Lunalify::Runtime {

    class Orchestrator {
    public:
        Orchestrator();
        ~Orchestrator();
        Errors::Code Run(DWORD ppid);
        static Errors::Code Spawn(const std::string& logPath, DWORD parentPid);
        void Stop();

    private:
        void CommandLoop();
        void EventLoop();

        void HandleCommand(Protocol::OpCode op, const std::string& payload);
        void SignalReadyToParent(DWORD ppid);


    private:
        std::atomic<bool> m_running{false};
        EventQueue m_eventQueue;

        std::thread m_commandThread;
        std::thread m_eventThread;
    };

}
