#ifndef JIL_EDITOR_LUA_PROXY_H_
#define JIL_EDITOR_LUA_PROXY_H_
#pragma once

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

class wxString;

namespace jil {
namespace editor {

// Bind classes, functions and variables to Lua.
void InitLua(lua_State* lua_state);

bool LoadLuaFile(lua_State* lua_state, const wxString& file, std::string* err_msg = NULL);

// Get global lua value with the given name.
luabridge::LuaRef GetLuaValue(lua_State* lua_state, const char* name);

// Get lua value with the given name under the global table 'ns'.
luabridge::LuaRef GetLuaValue(lua_State* lua_state, const char* ns, const char* name);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_LUA_PROXY_H_
