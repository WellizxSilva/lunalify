#pragma once
#include <windows.h>

namespace Lunalify::API {

struct AsyncEventData {
    OVERLAPPED overlapped;
    HANDLE hEvent;
    bool pending;
    char buffer[8192];

    AsyncEventData() : hEvent(NULL), pending(false) {
        ZeroMemory(&overlapped, sizeof(OVERLAPPED));
        /*
         * Create the event object for overlapped I/O completion
         *  (ensures event is valid while context exists)
         */
        hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        overlapped.hEvent = hEvent;
    }

    ~AsyncEventData() {
        if (hEvent) {
           CloseHandle(hEvent);
            hEvent = NULL;
        }
    }
};

class Context {
public:
    static Context& Get() {
        static Context instance;
        return instance;
    }

    void SetIsDaemon(bool value) { m_isDaemon = value; }
    bool IsDaemon() const { return m_isDaemon; }

    void SetDllInstance(HINSTANCE hInst) { m_hInst = hInst; }
    HINSTANCE GetDllInstance() const { return m_hInst; }

    HANDLE GetEventPipe() const { return m_eventPipe; }
    void SetEventPipe(HANDLE hPipe) {
        if (m_eventPipe != INVALID_HANDLE_VALUE) CloseHandle(m_eventPipe);
        m_eventPipe = hPipe;
    }
    void ResetEventPipe() {
        if (m_eventPipe != INVALID_HANDLE_VALUE) CloseHandle(m_eventPipe);
        m_eventPipe = INVALID_HANDLE_VALUE;
    }

    HANDLE GetCmdPipe() const { return m_cmdPipe; }
    void SetCmdPipe(HANDLE hPipe) {
        if (m_cmdPipe != INVALID_HANDLE_VALUE) CloseHandle(m_cmdPipe);
        m_cmdPipe = hPipe;
    }
    AsyncEventData* GetAsyncData() { return &m_asyncData; }
    void ResetCmdPipe() {
        if (m_cmdPipe != INVALID_HANDLE_VALUE) CloseHandle(m_cmdPipe);
        m_cmdPipe = INVALID_HANDLE_VALUE;
        m_asyncData.pending = false;
    }

private:
    Context() : m_isDaemon(false), m_hInst(NULL), m_eventPipe(INVALID_HANDLE_VALUE), m_cmdPipe(INVALID_HANDLE_VALUE) {}
    ~Context() { ResetEventPipe(); ResetCmdPipe(); }

    bool m_isDaemon;
    HINSTANCE m_hInst;
    HANDLE m_eventPipe;
    HANDLE m_cmdPipe;
    AsyncEventData m_asyncData;
};

}
