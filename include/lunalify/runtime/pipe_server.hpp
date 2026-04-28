#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include "../lunalify.hpp"

namespace Lunalify::Runtime {

    class PipeServer {
    public:
        enum class Mode { Inbound, Outbound, Duplex };

        PipeServer(std::wstring pipeName, Mode mode);
        ~PipeServer();
        // Blocks until the client connects.
        Errors::Code Listen();
        // Disconnects the active client.
        void Disconnect();
        // Reads exactly 'size' bytes. Returns false if the pipe breaks.
        Errors::Code ReadExact(
            uint8_t* buffer, size_t size);
        // Writes the bytes and flushes the pipe.
        Errors::Code Write(const std::vector<uint8_t>& data);
        bool IsValid() const;

    private:
        std::wstring m_pipeName;
        Mode m_mode;
        HANDLE m_hPipe;
    };

}
