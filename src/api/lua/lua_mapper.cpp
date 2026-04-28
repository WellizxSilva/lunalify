#include "lunalify/api/lua/lua_mapper.hpp"
#include "lunalify/api/types.hpp"
#include "lunalify/logger.hpp"

using namespace Lunalify::API::Lua;

ToastConfig Mapper::MapStackToToastConfig(lua_State* L) {
    ToastConfig config;
    if (!lua_istable(L, 1)) {
        LNF_ERROR("Lunalify::API::Lua::Mapper", "Expected table as first argument for toast config");
        return config;
    }
    auto GetStr = [&](const char* key, const char* def) {
        lua_getfield(L, 1, key);
        std::string res = luaL_optstring(L, -1, def);
        lua_pop(L, 1);
        return res;
    };
    auto GetBool = [&](const char* key, bool def) {
        lua_getfield(L, 1, key);
        bool res = lua_isboolean(L, -1) ? lua_toboolean(L, -1) : def;
        lua_pop(L, 1);
        return res;
    };
    config.title    = GetStr("title", "");
    config.message  = GetStr("message", "");
    config.appId    = GetStr("appId", "");
    config.iconPath = GetStr("iconPath", "");
    config.heroPath = GetStr("heroPath", "");
    config.sound    = GetStr("sound", "Default");
    config.duration = GetStr("duration", "short");
    config.scenario = GetStr("scenario", "default");
    config.tag      = GetStr("tag", "");
    config.group    = GetStr("group", "");
    config.silent   = GetBool("silent", false);
    lua_getfield(L, 1, "actions");
    if (lua_istable(L, -1)) FillActions(L, lua_gettop(L), config);
    lua_pop(L, 1);
    lua_getfield(L, 1, "inputs");
    if (lua_istable(L, -1)) FillInputs(L, lua_gettop(L), config);
    lua_pop(L, 1);
    lua_getfield(L, 1, "progress");
    if (lua_istable(L, -1)) FillProgress(L, lua_gettop(L), config);
    lua_pop(L, 1);
    return config;
}

void Mapper::FillActions(lua_State* L, int index, ToastConfig& config) {
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        ToastAction action;
        auto GetStr = [&](const char* f, const char* def) {
            lua_getfield(L, -1, f);
            std::string res = luaL_optstring(L, -1, def);
            lua_pop(L, 1);
            return res;
        };
        action.label = GetStr("label", "OK");
        action.target = GetStr("target", "");
        action.placement = GetStr("placement", "default");
        action.image = GetStr("image", "");
        action.type = GetStr("type", "protocol");
        action.style = GetStr("style", "");
        config.actions.push_back(action);
        lua_pop(L, 1);
    }
}

void Mapper::FillInputs(lua_State* L, int index, ToastConfig& config) {
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        ToastInput input;
        auto GetStr = [&](const char* f, const char* def) {
            lua_getfield(L, -1, f);
            std::string res = luaL_optstring(L, -1, def);
            lua_pop(L, 1);
            return res;
        };
        input.id = GetStr("id", "");
        input.type = GetStr("type", "text");
        input.placeholder = GetStr("placeholder", "");
        input.title = GetStr("title", "");
        input.defaultInput = GetStr("defaultInput", "");
        lua_getfield(L, -1, "selections");
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                ToastSelection sel;
                lua_getfield(L, -1, "id");
                sel.id = luaL_optstring(L, -1, "");
                lua_pop(L, 1);
                lua_getfield(L, -1, "content");
                sel.content = luaL_optstring(L, -1, "");
                lua_pop(L, 1);
                input.selections.push_back(sel);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
        config.inputs.push_back(input);
        lua_pop(L, 1);
    }
}

void Mapper::FillProgress(lua_State* L, int index, ToastConfig& config) {
    auto GetStr = [&](const char* f, const char* def) {
        lua_getfield(L, index, f);
        std::string res = luaL_optstring(L, -1, def);
        lua_pop(L, 1);
        return res;
    };

    lua_getfield(L, index, "active");
    config.progress.active = lua_toboolean(L, -1);
    lua_pop(L, 1);

    config.progress.title = GetStr("title", "");
    config.progress.value = GetStr("value", "0");
    config.progress.displayValue = GetStr("displayValue", "");
    config.progress.status = GetStr("status", "");
}

void Mapper::FillRegister(
    lua_State* L,
    int index,
    Lunalify::App::Lua::DataStructs::RegisterData& data,
    bool only_id_and_name) {
    auto GetStr = [&](const char* f, const char* def) {
        lua_getfield(L, index, f);
        std::string res = luaL_optstring(L, -1, def);
        lua_pop(L, 1);
        return res;
    };

    data.appId = GetStr("id", "");
    data.appName = GetStr("name", "");
    if (!only_id_and_name) {
        data.iconPath = GetStr("iconPath", "");
        data.daemonLogPath = GetStr("daemonLogPath", "");
    }
}

void Mapper::FillLogger(lua_State *L, int index, Lunalify::App::Lua::DataStructs::LoggerData &config) {
    (void)index;
    config.enabled = lua_toboolean(L, 1);
    config.level = Lunalify::Logger::ToLogLevel(luaL_optstring(L, 2, "info"));
    config.path = luaL_optstring(L, 3, "");
    config.to_console = lua_toboolean(L, 4);
};

void Mapper::FillUpdate(lua_State *L, int index, ToastUpdate &config) {
    int absIndex = lua_absindex(L, index);
    auto GetStr = [&](const char* f, const char* def) {
        lua_getfield(L, absIndex, f);
        std::string res = luaL_optstring(L, -1, def);
        lua_pop(L, 1);
        return res;
    };
    config.appId = GetStr("appId", "");
    config.tag = GetStr("tag", "");
    config.title = GetStr("title", "");
    config.value = GetStr("value", "0");
    config.vString = GetStr("displayValue", "");
    config.status = GetStr("status", "");
}
