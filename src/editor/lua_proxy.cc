#include "editor/lua_proxy.h"
#include <string>
#include "wx/string.h"
#include "editor/file_io.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"

namespace jil {
namespace editor {

void InitLua(lua_State* lua_state) {
  luabridge::getGlobalNamespace(lua_state)
    .beginClass<OptionValue>("Option")
    .addConstructor<void(*)()>()
    .addFunction("asNumber", &OptionValue::AsInt)
    .addFunction("asString", &OptionValue::AsString)
    .addFunction("asBool", &OptionValue::AsBool)
    .endClass()
    .beginClass<TextPoint>("Point")
    .addConstructor<void(*)(Coord, Coord)>()
    .addData("x", &TextPoint::x)
    .addData("y", &TextPoint::y)
    .addFunction("set", &TextPoint::Set)
    .addFunction("reset", &TextPoint::Reset)
    .addFunction("valid", &TextPoint::Valid)
    .endClass()
    .beginClass<TextLine>("Line")
    .addFunction("getLength", &TextLine::Length)
    .addFunction("getTabbedLength", &TextLine::TabbedLength)
    .addFunction("isChar", &TextLine::Lua_isChar)
    .addFunction("isEmpty", &TextLine::IsEmpty)
    .addFunction("findNonSpace", &TextLine::FindNonSpace)
    .addFunction("findLastNonSpace", &TextLine::FindLastNonSpace)
    .addFunction("findLastChar", &TextLine::Lua_findLastChar)
    .addFunction("equal", &TextLine::Lua_equal)
    .addFunction("getUnpairedLeftKey", &TextLine::Lua_getUnpairedLeftKey)
    .addFunction("getIndent", &TextLine::GetIndent)
    .addFunction("isEolEscaped", &TextLine::IsEolEscaped)
    .addFunction("isCommentOnly", &TextLine::IsCommentOnly)
    .addFunction("isComment", &TextLine::IsComment)
    .addFunction("isString", &TextLine::IsString)
    .addFunction("isCommentOrString", &TextLine::IsCommentOrString)
    .addCFunction("startWith", &TextLine::Lua_startWith)  
    .addCFunction("endWith", &TextLine::Lua_endWith)
    .endClass()
    .beginClass<TextBuffer>("Buffer")
    .addFunction("getTabStop", &TextBuffer::tab_stop)
    .addFunction("getIndentOption", &TextBuffer::GetIndentOption)
    .addFunction("getLineCount", &TextBuffer::LineCount)
    .addFunction("getLine", &TextBuffer::Line)
    .addFunction("getIndent", &TextBuffer::GetIndent)
    .addFunction("getPrevLineIndent", &TextBuffer::GetPrevLineIndent)
    .addFunction("getPrevNonEmptyLine", &TextBuffer::PrevNonEmptyLine)
    .addFunction("getUnpairedLeftKey", &TextBuffer::Lua_UnpairedLeftKey)
    .endClass();
} 
 
bool LoadLuaFile(lua_State* lua_state,
                 const wxString& file,
                 std::string* lua_error) {
  std::string bytes;
  if (ReadBytes(file, &bytes) != 0) {
    return false;
  }

  int err = luaL_dostring(lua_state, bytes.c_str());
  if (err != LUA_OK) {
    if (lua_error != NULL) {
      // Get the error message from stack top.
      if (lua_gettop(lua_state) != 0) {
        *lua_error = lua_tostring(lua_state, -1);
      }
    }
    return false;
  }

  return true;
}

luabridge::LuaRef GetLuaValue(lua_State* lua_state, const char* name) {
  return luabridge::getGlobal(lua_state, name);
}

luabridge::LuaRef GetLuaValue(lua_State* lua_state, const char* ns, const char* name) {
  luabridge::LuaRef ns_ref = luabridge::getGlobal(lua_state, ns);
  if (!ns_ref.isNil() && ns_ref.isTable()) {
    return ns_ref[name];
  }
  return luabridge::LuaRef(lua_state);
}

}  // namespace editor
}  // namespace jil
