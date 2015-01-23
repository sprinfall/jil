#include "app/lua_proxy.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

#include "wx/log.h"

#include "editor/file_io.h"
#include "app/util.h"

namespace jil {

LuaProxy::LuaProxy() {
  state_ = luaL_newstate();
  luaL_openlibs(state_);
}

LuaProxy::~LuaProxy() {
  lua_close(state_);
}

void LuaProxy::Init() {
  wxString lua_file = path::ResourceDir() + wxT("test.lua");
  std::string bytes;
  if (editor::ReadBytes(lua_file, &bytes) != 0) {
    //wxLogError(wxT("Failed to open file: %s"), lua_file);
    return;
  }

  int err = luaL_dostring(state_, bytes.c_str());
  if (err != 0) {
    wxLogError(wxT("Failed to run the Lua script: %s"), lua_file);
    return;
  }

  luabridge::LuaRef add = luabridge::getGlobal(state_, "add");
  if (add.isNil()) {
    wxLogError("Cannot get the 'add' function!");
    return;
  }

  int result = add(1, 2);
  wxLogDebug("add result: %d", result);
}

}  // namespace jil
