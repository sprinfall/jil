#ifndef JIL_LUA_PROXY_H_
#define JIL_LUA_PROXY_H_
#pragma once

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

class wxString;

namespace jil {

class LuaProxy {
public:
  LuaProxy();
  ~LuaProxy();

  lua_State* state() const {
    return state_;
  }

  void Init();

  // Load the indent lua file, return the indent function.
  luabridge::LuaRef GetIndentFunc(const wxString& indent_file);

private:
  lua_State* state_;
};

}  // namespace jil

#endif  // JIL_LUA_PROXY_H_
