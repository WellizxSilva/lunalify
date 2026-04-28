#pragma once
#include <windows.h>

namespace Lunalify::API {

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
    void ResetCmdPipe() {
        if (m_cmdPipe != INVALID_HANDLE_VALUE) CloseHandle(m_cmdPipe);
        m_cmdPipe = INVALID_HANDLE_VALUE;
    }

private:
    Context() : m_isDaemon(false), m_hInst(NULL), m_eventPipe(INVALID_HANDLE_VALUE), m_cmdPipe(INVALID_HANDLE_VALUE) {}
    ~Context() { ResetEventPipe(); ResetCmdPipe(); }

    bool m_isDaemon;
    HINSTANCE m_hInst;
    HANDLE m_eventPipe;
    HANDLE m_cmdPipe;
};

}
