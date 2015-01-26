#include "app/lua_proxy.h"

#include "wx/log.h"
#include "wx/msgdlg.h"
#include "wx/string.h"

#include "editor/file_io.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "app/util.h"
#include "app/i18n_strings.h"

namespace jil {

LuaProxy::LuaProxy() {
  state_ = luaL_newstate();
  luaL_openlibs(state_);
}

LuaProxy::~LuaProxy() {
  lua_close(state_);
}

void LuaProxy::Init() {
  using namespace editor;

  // Export TextBuffer to Lua.
  luabridge::getGlobalNamespace(state_)
    .beginClass<TextPoint>("Point")
      .addConstructor<void (*)(Coord, Coord)>()
      .addData("x", &TextPoint::x)
      .addData("y", &TextPoint::y)
      .addFunction("set", &TextPoint::Set)
      .addFunction("reset", &TextPoint::Reset)
      .addFunction("isvalid", &TextPoint::Valid)
    .endClass()
    .beginClass<TextLine>("Line")
      .addFunction("getlength", &TextLine::Length)
      .addFunction("ischar", &TextLine::ischar)
      .addFunction("getindent", &TextLine::GetIndent)
      .addFunction("isempty", &TextLine::IsEmpty)
      .addFunction("firstnonspacechar", &TextLine::FirstNonSpaceChar)
      .addFunction("lastnonspacechar", &TextLine::LastNonSpaceChar)
      .addCFunction("startwith", &TextLine::startwith)
      .addCFunction("endwith", &TextLine::endwith)
    .endClass()
    .beginClass<TextBuffer>("Buffer")
      .addFunction("getlinecount", &TextBuffer::LineCount)
      .addFunction("getline", &TextBuffer::Line)
      .addFunction("prevnonemptyline", &TextBuffer::PrevNonEmptyLine)
      .addFunction("getindent", &TextBuffer::GetIndent)
      .addFunction("unpairedleftkey", &TextBuffer::unpairedleftkey)
    .endClass();
}

luabridge::LuaRef LuaProxy::GetIndentFunc(const wxString& indent_file) {
  std::string bytes;
  if (editor::ReadBytes(indent_file, &bytes) != 0) {
    wxLogError(wxT("Failed to read file: %s"), indent_file);
    return luabridge::LuaRef(state_);
  }

  int err = luaL_dostring(state_, bytes.c_str());

  if (err != LUA_OK) {
    wxString msg;
    if (lua_gettop(state_) != 0) {
      msg = lua_tostring(state_, -1);
    }

    wxMessageBox(msg, kTrError, wxOK | wxCENTRE | wxICON_ERROR, NULL);

    return luabridge::LuaRef(state_);
  }

  luabridge::LuaRef indent_func = luabridge::getGlobal(state_, "indent");
  if (indent_func.isNil() || !indent_func.isFunction()) {
    wxString msg = wxT("Can't get indent function!");
    wxMessageBox(msg, kTrError, wxOK | wxCENTRE | wxICON_ERROR, NULL);

    return luabridge::LuaRef(state_);
  }

  return indent_func;
}

}  // namespace jil
