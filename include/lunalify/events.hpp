#include "lunalify.hpp"
#include <string>
#include <sstream>
#include <vector>

void PushEventToLua(lua_State* L, const std::string& rawMessage) {

    std::stringstream ss(rawMessage);
    std::string name, id, args;

    std::getline(ss, name, Lunalify::Protocol::SEPARATOR);
    std::getline(ss, id, Lunalify::Protocol::SEPARATOR);
    std::getline(ss, args);

    std::string eventName = "unknown";
    if (name == Lunalify::Protocol::EVENT_ACTIVATED) eventName = "activated";
    else if (name == Lunalify::Protocol::EVENT_DISMISSED) eventName = "dismissed";

    lua_newtable(L);
    lua_pushstring(L, eventName.c_str());
    lua_setfield(L, -2, "event");

    lua_pushstring(L, id.c_str());
    lua_setfield(L, -2, "id");

    lua_pushstring(L, args.c_str());
    lua_setfield(L, -2, "args");
}
