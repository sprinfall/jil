#ifndef EDITOR_LUA_PROXY_H_
#define EDITOR_LUA_PROXY_H_

#include <string>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

class wxString;

namespace editor {

// Bind classes, functions and variables to Lua.
void InitLua(lua_State* lua_state);

bool LoadLuaFile(lua_State* lua_state,
                 const wxString& file,
                 std::string* lua_error = NULL);

// Get global lua value with the given name.
luabridge::LuaRef GetLuaValue(lua_State* lua_state, const char* name);

// Get lua value with the given name under the global table 'ns'.
luabridge::LuaRef GetLuaValue(lua_State* lua_state, const char* ns,
                              const char* name);

}  // namespace editor

#endif  // EDITOR_LUA_PROXY_H_
