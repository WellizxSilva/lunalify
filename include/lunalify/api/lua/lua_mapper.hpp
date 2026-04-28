#pragma once
#include <lua.hpp>

#include "../types.hpp"

using namespace Lunalify::API::Toast;
using namespace Lunalify::App::Lua::DataStructs;

namespace Lunalify::API::Lua::Mapper {
    ToastConfig MapStackToToastConfig(lua_State* L);
    void FillActions(lua_State* L, int index, ToastConfig& config);
    void FillInputs(lua_State* L, int index, ToastConfig& config);
    void FillProgress(lua_State* L, int index, ToastConfig& config);
    void FillLogger(lua_State* L, int index, LoggerData& config);
    void FillUpdate(lua_State* L, int index, ToastUpdate& config);

    void FillRegister(
        lua_State* L,
        int index,
        RegisterData& data,
        bool only_id_and_name);
}
