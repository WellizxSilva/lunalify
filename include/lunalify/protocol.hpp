#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cstring>

namespace Lunalify::Protocol {
    inline const wchar_t* CMD_PIPE_NAME = L"\\\\.\\pipe\\LunalifyCommands";
    inline const wchar_t* EVENT_PIPE_NAME = L"\\\\.\\pipe\\LunalifyEvents";
    inline const char SEPARATOR = '\x1f'; // Unit separator

    constexpr uint32_t MAGIC_HEADER = 0x4C4E4659; // "LNFY"
    enum class OpCode : uint32_t {
        CMD_FIRE_TOAST = 0x01,
        CMD_UPDATE_TOAST = 0x02,
        // CMD_EXIT = 0x03,
        CMD_SHUTDOWN = 0xFF,

        EVENT_ACTIVATED= 0x10,
        EVENT_DISMISSED = 0x11,
        EVENT_FAILED = 0x12,
        EVENT_DISPATCH = 0x13,

        Acknowledge = 0xAA
    };
    #pragma pack(push, 1)
    struct PacketHeader {
        uint32_t magic;
        OpCode   opCode;
        uint32_t payloadSize;
    };
    #pragma pack(pop)

    inline std::vector<uint8_t> Pack(OpCode op, const std::string& payload = "") {
        std::vector<uint8_t> buffer(sizeof(PacketHeader) + payload.size());
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer.data());
        header->magic = MAGIC_HEADER;
        header->opCode = op;
        header->payloadSize = static_cast<uint32_t>(payload.size());
        if (!payload.empty()) {
            std::memcpy(buffer.data() + sizeof(PacketHeader), payload.data(), payload.size());
        }
        return buffer;
    }
}
