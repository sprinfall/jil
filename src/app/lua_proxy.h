#ifndef JIL_LUA_PROXY_H_
#define JIL_LUA_PROXY_H_
#pragma once

struct lua_State;

namespace jil {

class LuaProxy {
public:
  LuaProxy();
  ~LuaProxy();

  void Init();

private:
  lua_State* state_;
};

}  // namespace jil

#endif  // JIL_LUA_PROXY_H_
