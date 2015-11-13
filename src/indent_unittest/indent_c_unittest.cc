#include <memory>

extern "C" {
#include "lualib.h"
}
#include "editor/lua_proxy.h"

#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"

#include "gtest/gtest.h"

using namespace jil::editor;

typedef std::auto_ptr<TextBuffer> TextBufferPtr;

static const Encoding kEncoding = EncodingFromName(ENCODING_NAME_ISO_8859_1);

class IndentCTest : public testing::Test {
protected:
  virtual void SetUp() {
    lua_state_ = luaL_newstate();
    luaL_openlibs(lua_state_);
    InitLua(lua_state_);

    ft_plugin_ = new FtPlugin(FileType(wxT("c"), wxT("C")), lua_state_);

    TextOptions text_options;
    text_options.expand_tab = true;
    text_options.tab_stop = 4;
    text_options.shift_width = 4;
    text_options.delimiters = L"!@#%^$&*()+-=\\|/?[]{}<>,.;:'\"`~";
    ft_plugin_->set_text_options(text_options);

    // Indent functions relies on these quotes.
    // It would be better to load from the lex file instead.

    Quote* quote1 = new Quote(Lex(kLexComment), L"/*", L"*/", kQuoteMultiLine);
    ft_plugin_->AddQuote(quote1);

    Quote* quote2 = new Quote(Lex(kLexComment), L"//", L"", kQuoteEscapeEol);
    ft_plugin_->AddQuote(quote2);

    Quote* quote_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", kQuoteEscapeEol);
    ft_plugin_->AddQuote(quote_str);

    Quote* quote_char = new Quote(Lex(kLexConstant, kLexConstantChar), L"'", L"'", 0);
    ft_plugin_->AddQuote(quote_char);

    // TODO
    // jil/build/src/editor
    wxString cwd = wxGetCwd();
    wxString ftplugin_dir = "../../../data/jilfiles/ftplugin/c/";
    wxSetWorkingDirectory(ftplugin_dir);

    if (LoadLuaFile(lua_state_, wxT("indent.lua"))) {
      luabridge::LuaRef indent_func = GetLuaValue(lua_state_, "c", "indent");
      ft_plugin_->set_indent_func(indent_func);
    }

    wxSetWorkingDirectory(cwd);

    buffer_ = TextBuffer::Create(0, ft_plugin_, kEncoding);
  }

  virtual void TearDown() {
    delete buffer_;
    delete ft_plugin_;
    lua_close(lua_state_);
  }

  Coord GetExpectedIndent(Coord ln) {
    const luabridge::LuaRef& indent_func = ft_plugin_->indent_func();
    if (indent_func.isNil() || !indent_func.isFunction()) {
      return 0;
    }
    return indent_func(buffer_, ln);
  }

  lua_State* lua_state_;
  FtPlugin* ft_plugin_;
  TextBuffer* buffer_;
};

#define ASSERT_LINE(ln)\
  EXPECT_EQ(buffer_->GetIndent(ln), GetExpectedIndent(ln));

////////////////////////////////////////////////////////////////////////////////

// Don't have to provide complete cases since it reuses C++'s indent function.

TEST_F(IndentCTest, FunctionDef_OneLineParams) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCTest, FunctionDef_MultiLineParams) {
  buffer_->AppendLine(L"void add(int a,");
  buffer_->AppendLine(L"         int b,");
  buffer_->AppendLine(L"         int c) {");
  buffer_->AppendLine(L"    test;");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentCTest, Macro_EolEscaped_IndentBody) {
  buffer_->SetIndentOption("indent_preproc_body", OptionValue::FromBool(true));

  buffer_->AppendLine(L"        int i;");
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCTest, Comments) {
  buffer_->AppendLine(L"int i;");
  buffer_->AppendLine(L"/*if (a > b) {");
  buffer_->AppendLine(L"comments");
  buffer_->AppendLine(L"comments");
  buffer_->AppendLine(L"comments*/");
  buffer_->AppendLine(L"return a,");
   
  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}
