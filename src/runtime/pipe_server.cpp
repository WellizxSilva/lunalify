#include "lunalify/runtime/pipe_server.hpp"
#include "lunalify/errors.hpp"
#include "windows.h"
#include <synchapi.h>

namespace Lunalify::Runtime {

    PipeServer::PipeServer(std::wstring pipeName, Mode mode)
        : m_pipeName(std::move(pipeName)), m_mode(mode), m_hPipe(INVALID_HANDLE_VALUE)
    {
        DWORD openMode = 0;
        if (m_mode == Mode::Inbound) openMode = PIPE_ACCESS_INBOUND;
        else if (m_mode == Mode::Outbound) openMode = PIPE_ACCESS_OUTBOUND;
        else openMode = PIPE_ACCESS_DUPLEX;
        LNF_INFO("Lunalify:Runtime:PipeServer", "Creating Named Pipe ...");
        // Create the pipe in the constructor
        m_hPipe = CreateNamedPipeW(
            m_pipeName.c_str(),
            openMode,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE| PIPE_WAIT,
            // For now lunalify is single-instance, but leaving it unlimited improves reconnection
            PIPE_UNLIMITED_INSTANCES,
            8192,       // Out buffer
            8192,       // In buffer
            0,          // Default timeout
            NULL
        );
        if (m_hPipe == INVALID_HANDLE_VALUE) {
            LNF_ERROR("Lunalify:Runtime:PipeServer", "Failed to create Named Pipe. Error: {} ", GetLastError());
        }
    }

    PipeServer::~PipeServer() {
        if (IsValid()) {
            DisconnectNamedPipe(m_hPipe);
            CloseHandle(m_hPipe);
        }
    }

    bool PipeServer::IsValid() const {
        return m_hPipe != INVALID_HANDLE_VALUE;
    }

    Errors::Code PipeServer::Listen() {
        if (!IsValid()) return Errors::Code::IPC_DAEMON_NOT_FOUND;

        BOOL connected = ConnectNamedPipe(m_hPipe, NULL);
        if (connected || GetLastError() == ERROR_PIPE_CONNECTED) {
            return Errors::Code::SUCCESS;
        }
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_BUSY) return Errors::Code::IPC_PIPE_BUSY;
        return Errors::Code::IPC_CONNECTION_LOST;
    }

    void PipeServer::Disconnect() {
        if (IsValid()) {
            DisconnectNamedPipe(m_hPipe);
        }
    }

    Errors::Code PipeServer::ReadExact(uint8_t* buffer, size_t size) {
        if (!IsValid()) return Errors::Code::IPC_DAEMON_NOT_FOUND;
        if (size == 0) return Errors::Code::SUCCESS;

        DWORD totalRead = 0;
        while (totalRead < size) {
            DWORD bytesRead = 0;
            BOOL success = ReadFile(
                m_hPipe,
                buffer + totalRead,
                static_cast<DWORD>(size - totalRead),
                &bytesRead,
                NULL
            );
            DWORD err = GetLastError();
            // "ERROR_MORE_DATA" is expected when the buffer is too small, not an error
            if ((!success && err != ERROR_MORE_DATA) || bytesRead == 0) {
                LNF_ERROR("Lunalify:PipeServer", "Read error. Bytes read: {}, Error: {}", bytesRead, err);
                if (err == ERROR_BROKEN_PIPE) return Errors::Code::IPC_CONNECTION_LOST;
                return Errors::Code::IPC_PROTOCOL_MISMATCH;
            }
            totalRead += bytesRead;
        }
        return Errors::Code::SUCCESS;
    }

    Errors::Code PipeServer::Write(const std::vector<uint8_t>& data) {
        if (!IsValid()) return Errors::Code::IPC_DAEMON_NOT_FOUND;
        if (data.empty()) return Errors::Code::SUCCESS;

        DWORD bytesWritten = 0;
        BOOL success = WriteFile(
            m_hPipe,
            data.data(),
            static_cast<DWORD>(data.size()),
            &bytesWritten,
            NULL
        );

        if (success && bytesWritten == data.size()) {
            FlushFileBuffers(m_hPipe);
            return Errors::Code::SUCCESS;
        }
        return Errors::Code::IPC_CONNECTION_LOST;
    }

}
