#include "editor_unittest/indent_test_base.h"

using namespace editor;

static const Encoding kEncoding = GetEncodingById(ENCODING_ISO_8859_1);

lua_State* IndentTestBase::lua_state;
FtPlugin* IndentTestBase::ft_plugin;

void IndentTestBase::LoadLua() {
  lua_state = luaL_newstate();
  luaL_openlibs(lua_state);
  InitLua(lua_state);
}

void IndentTestBase::SetTextOptions(bool expand_tab, int tab_stop) {
  TextOptions text_options;

  text_options.expand_tab = expand_tab;
  text_options.tab_stop = tab_stop;

  text_options.delimiters = L"!@#%^$&*()+-=\\|/?[]{}<>,.;:'\"`~";

  ft_plugin->set_text_options(text_options);
}

void IndentTestBase::LoadIndentFunc(const wxString& ft_id) {
  // TODO
  // jil/build/src/editor
  wxString ftplugin_dir = wxT("../../../data/jilfiles/ftplugin/") + ft_id;

  wxString cwd = wxGetCwd();
  wxSetWorkingDirectory(ftplugin_dir);
  
  std::string err;
  if (LoadLuaFile(lua_state, wxT("indent.lua"), &err)) {
    std::string ns = ft_id.ToAscii().data();
    luabridge::LuaRef indent_func = GetLuaValue(lua_state, ns.c_str(), "indent");
    ft_plugin->set_indent_func(indent_func);
  }

  wxSetWorkingDirectory(cwd);
}

void IndentTestBase::StaticSetUp(const FileType& ft) {
  LoadLua();
  ft_plugin = new FtPlugin(ft, lua_state);
  LoadIndentFunc(ft.id);
}

void IndentTestBase::StaticTearDown() {
  delete ft_plugin;
  lua_close(lua_state);
}

void IndentTestBase::SetUp() {
  buffer_ = new TextBuffer(0, ft_plugin, kEncoding);
}

void IndentTestBase::TearDown() {
  delete buffer_;
}

int IndentTestBase::GetExpectedIndent(int ln) {
  const luabridge::LuaRef& indent_func = ft_plugin->indent_func();
  if (indent_func.isNil() || !indent_func.isFunction()) {
    return 0;
  }
  return indent_func(buffer_, ln);
}
