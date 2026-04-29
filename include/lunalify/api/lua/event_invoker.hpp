#pragma once
#include <lua.hpp>
#include <vector>
#include <string>
#include <sstream>
#include "lunalify/lunalify.hpp"

namespace Lunalify::API::Lua::Invokers {

    inline int PushEvent(lua_State* L, const char* buffer, DWORD bytesRead) {
        if (bytesRead < sizeof(Lunalify::Protocol::PacketHeader)) {
            lua_pushnil(L);
            return 1;
        }

        auto* header = reinterpret_cast<const Lunalify::Protocol::PacketHeader*>(buffer);

        if (header->magic != Lunalify::Protocol::MAGIC_HEADER) {
            lua_pushnil(L);
            return 1;
        }

        // payload start right after the header
        std::string payload(buffer + sizeof(Lunalify::Protocol::PacketHeader), header->payloadSize);

        std::vector<std::string> parts;
        std::stringstream ss(payload);
        std::string item;
        while (std::getline(ss, item, Lunalify::Protocol::SEPARATOR)) {
            parts.push_back(item);
        }

        if (parts.size() >= 3) {
            lua_newtable(L);
            lua_pushstring(L, parts[0].c_str()); lua_setfield(L, -2, "event");
            lua_pushstring(L, parts[1].c_str()); lua_setfield(L, -2, "id");
            lua_pushstring(L, parts[2].c_str()); lua_setfield(L, -2, "args");
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }
}
