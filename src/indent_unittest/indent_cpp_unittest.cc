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

class IndentCppTest : public testing::Test {
protected:
  virtual void SetUp() {
    lua_state_ = luaL_newstate();
    luaL_openlibs(lua_state_);
    InitLua(lua_state_);

    ft_plugin_ = new FtPlugin(FileType(wxT("cpp"), wxT("C++")), lua_state_);

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
    //wxString cwd = wxGetCwd();
    wxString ftplugin_dir = "../../../data/jilfiles/ftplugin/";
    wxString indent_file = ftplugin_dir + wxT("cpp/indent.lua");

    if (LoadLuaFile(lua_state_, indent_file)) {
      luabridge::LuaRef indent_func = GetLuaValue(lua_state_, "indent");
      ft_plugin_->set_indent_func(indent_func);
    }

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

TEST_F(IndentCppTest, IsPreprocHead) {
  luabridge::LuaRef is_preproc_head = GetLuaValue(lua_state_, "isPreprocHead");
  if (is_preproc_head.isNil() || !is_preproc_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"#define MAX_SIZE 256");
  buffer_->AppendLine(L"");
  buffer_->AppendLine(L"    ");
  buffer_->AppendLine(L"int i = 1;");

  EXPECT_TRUE(is_preproc_head(buffer_->Line(2)).cast<bool>());
  EXPECT_FALSE(is_preproc_head(buffer_->Line(3)).cast<bool>());
  EXPECT_FALSE(is_preproc_head(buffer_->Line(4)).cast<bool>());
  EXPECT_FALSE(is_preproc_head(buffer_->Line(5)).cast<bool>());
}

TEST_F(IndentCppTest, IsPreprocBody) {
  luabridge::LuaRef is_preproc_body = GetLuaValue(lua_state_, "isPreprocBody");
  if (is_preproc_body.isNil() || !is_preproc_body.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");

  EXPECT_FALSE(is_preproc_body(buffer_, 2).cast<bool>());
  EXPECT_TRUE(is_preproc_body(buffer_, 3).cast<bool>());
}

TEST_F(IndentCppTest, IsPreproc) {
  luabridge::LuaRef is_preproc = GetLuaValue(lua_state_, "isPreproc");
  if (is_preproc.isNil() || !is_preproc.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");
  buffer_->AppendLine(L"int i;");

  EXPECT_TRUE(is_preproc(buffer_, 2).cast<bool>());
  EXPECT_TRUE(is_preproc(buffer_, 3).cast<bool>());
  EXPECT_FALSE(is_preproc(buffer_, 4).cast<bool>());
}

TEST_F(IndentCppTest, GetPrevLine) {
  // getPrevLine(buffer, ln, skip_comment, skip_preproc)
  luabridge::LuaRef get_prev_line = GetLuaValue(lua_state_, "getPrevLine");
  if (get_prev_line.isNil() || !get_prev_line.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"    int j;");           // 2
  buffer_->AppendLine(L"// comments");          // 3: Preproc
  buffer_->AppendLine(L"#define MAX_SIZE \\");  // 4: Preproc
  buffer_->AppendLine(L"    256");              // 5: Preproc
  buffer_->AppendLine(L"    ");                 // 6: Empty line
  buffer_->AppendLine(L"int i;");               // 7

  EXPECT_EQ(0, get_prev_line(buffer_, 2, false, false).cast<int>());
  EXPECT_EQ(2, get_prev_line(buffer_, 3, false, false).cast<int>());
  EXPECT_EQ(2, get_prev_line(buffer_, 3, true, true).cast<int>());
  EXPECT_EQ(3, get_prev_line(buffer_, 4, false, false).cast<int>());
  EXPECT_EQ(2, get_prev_line(buffer_, 4, true, false).cast<int>());
  EXPECT_EQ(4, get_prev_line(buffer_, 5, false, false).cast<int>());
  EXPECT_EQ(4, get_prev_line(buffer_, 5, true, false).cast<int>());
  EXPECT_EQ(2, get_prev_line(buffer_, 5, true, true).cast<int>());
  EXPECT_EQ(5, get_prev_line(buffer_, 6, false, false).cast<int>());
  EXPECT_EQ(5, get_prev_line(buffer_, 6, true, false).cast<int>());
  EXPECT_EQ(3, get_prev_line(buffer_, 6, false, true).cast<int>());
  EXPECT_EQ(2, get_prev_line(buffer_, 6, true, true).cast<int>());
}

TEST_F(IndentCppTest, GetPrevLineIndent) {
  // getPrevLineIndent(buffer, ln, skip_comment, skip_preproc)
  luabridge::LuaRef get_prev_line_indent = GetLuaValue(lua_state_, "getPrevLineIndent");
  if (get_prev_line_indent.isNil() || !get_prev_line_indent.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"    int j;");             // 2
  buffer_->AppendLine(L"  // comments");          // 3: Preproc
  buffer_->AppendLine(L" #define MAX_SIZE \\");   // 4: Preproc
  buffer_->AppendLine(L"      256");              // 5: Preproc
  buffer_->AppendLine(L"    ");                   // 6: Empty line
  buffer_->AppendLine(L"int i;");                 // 7

  EXPECT_EQ(0, get_prev_line_indent(buffer_, 2, false, false).cast<int>());
  EXPECT_EQ(4, get_prev_line_indent(buffer_, 3, false, false).cast<int>());
  EXPECT_EQ(4, get_prev_line_indent(buffer_, 3, true, true).cast<int>());
  EXPECT_EQ(2, get_prev_line_indent(buffer_, 4, false, false).cast<int>());
  EXPECT_EQ(4, get_prev_line_indent(buffer_, 4, true, false).cast<int>());
  EXPECT_EQ(1, get_prev_line_indent(buffer_, 5, false, false).cast<int>());
  EXPECT_EQ(1, get_prev_line_indent(buffer_, 5, true, false).cast<int>());
  EXPECT_EQ(4, get_prev_line_indent(buffer_, 5, true, true).cast<int>());
  EXPECT_EQ(6, get_prev_line_indent(buffer_, 6, false, false).cast<int>());
  EXPECT_EQ(6, get_prev_line_indent(buffer_, 6, true, false).cast<int>());
  EXPECT_EQ(2, get_prev_line_indent(buffer_, 6, false, true).cast<int>());
  EXPECT_EQ(4, get_prev_line_indent(buffer_, 6, true, true).cast<int>());
}

TEST_F(IndentCppTest, GetPrevLineStartWith) {
  luabridge::LuaRef get_prev_line_start_with = GetLuaValue(lua_state_, "getPrevLineStartWith");
  if (get_prev_line_start_with.isNil() || !get_prev_line_start_with.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"public:");
  buffer_->AppendLine(L"protected:");
  buffer_->AppendLine(L"private:");
  buffer_->AppendLine(L"   ");

  // NOTE: Use const char* instead of const wchar_t*.
  EXPECT_EQ(2, get_prev_line_start_with(buffer_, 5, "public").cast<int>());
  EXPECT_EQ(3, get_prev_line_start_with(buffer_, 5, "protected").cast<int>());
  EXPECT_EQ(4, get_prev_line_start_with(buffer_, 5, "private").cast<int>());
  EXPECT_EQ(4, get_prev_line_start_with(buffer_, 5, "public", "protected", "private").cast<int>());
  EXPECT_EQ(4, get_prev_line_start_with(buffer_, 5, "protected", "public", "private").cast<int>());
  EXPECT_EQ(4, get_prev_line_start_with(buffer_, 5, "private", "public", "protected").cast<int>());
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(IndentCppTest, Namespace_Indent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(true));

  buffer_->AppendLine(L"namespace {");
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, Namespace_DontIndent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(false));

  buffer_->AppendLine(L"namespace {");
  buffer_->AppendLine(L"int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, Namespace_NewLineBrace_Indent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(true));

  buffer_->AppendLine(L"namespace");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, Namespace_NewLineBrace_DontIndent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(false));

  buffer_->AppendLine(L"namespace");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, SimpleBraceBlock) {
  buffer_->AppendLine(L"int a, b, c;");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    int sum = a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, SimpleBraceBlock_EmptyBody) {
  buffer_->AppendLine(L"int a, b, c;");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, FunctionDef_EmptyBody) {
  buffer_->AppendLine(L"void None() {");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
}

TEST_F(IndentCppTest, FunctionDef_EmptyBody_NewLineBrace) {
  buffer_->AppendLine(L"void None()");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, FunctionDef_OneLineParams) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, FunctionDef_MultiLineParams) {
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

TEST_F(IndentCppTest, FunctionDef_MultiLineParams2) {
  buffer_->AppendLine(L"\t void add(int a,");  // Note the '\t'.
  buffer_->AppendLine(L"              int b,");
  buffer_->AppendLine(L"              int c) {");
  buffer_->AppendLine(L"         test;");
  buffer_->AppendLine(L"         return a + b + c;");
  buffer_->AppendLine(L"     }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentCppTest, FunctionDef_OneLineParams_NewLineBrace) {
  buffer_->AppendLine(L"void add(int a, int b, int c)");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, FunctionDef_MultiLineParams_NewLineBrace) {
  buffer_->AppendLine(L"void add(int a,");
  buffer_->AppendLine(L"         int b,");
  buffer_->AppendLine(L"         int c)");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentCppTest, FunctionCall_StringParenthesis) {
  buffer_->AppendLine(L"TextPoint p = buffer->UnpairedLeftKey(TextPoint(j, prev_ln),");
  buffer_->AppendLine(L"                                      L'(',");
  buffer_->AppendLine(L"                                      L')');");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, If_OneLineConditions) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, If_NoBrace) {
  buffer_->AppendLine(L"if (a > b)");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, If_NoBrace_WithElse) {
  buffer_->AppendLine(L"if (a > b)");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"else");
  buffer_->AppendLine(L"    return a;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentCppTest, If_NoBrace_OneLine) {
  buffer_->AppendLine(L"if (a > b) return b;");
  buffer_->AppendLine(L"else return a;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, For_NoBrace) {
  buffer_->AppendLine(L"for (int i = 0; i < count; ++i)");
  buffer_->AppendLine(L"    sum += i;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, For_MultiLine) {
  buffer_->AppendLine(L"for (int i = 0;");
  buffer_->AppendLine(L"     i < count;");
  buffer_->AppendLine(L"     ++i) {");
  buffer_->AppendLine(L"    sum += i;");
  buffer_->AppendLine(L"    test;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentCppTest, While_NoBrace) {
  buffer_->AppendLine(L"while (i < count)");
  buffer_->AppendLine(L"    sum += i;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, While_NoBrace_OnLine) {
  buffer_->AppendLine(L"while (i < count) sum += i;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
}

TEST_F(IndentCppTest, Class) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, Class_PublicAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");

  ASSERT_LINE(3);
}

TEST_F(IndentCppTest, Class_ProtectedAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    protected:");

  ASSERT_LINE(3);
}

TEST_F(IndentCppTest, Class_PrivateAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    private:");

  ASSERT_LINE(3);
}

TEST_F(IndentCppTest, Class_EmptyAccessors) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentCppTest, Class_Accessors) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"        ~A();");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"        A();");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
}

TEST_F(IndentCppTest, Struct_EmptyAccessors) {
  buffer_->AppendLine(L"    struct A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentCppTest, Struct_Accessors) {
  buffer_->AppendLine(L"    struct A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"        ~A();");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"        A();");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
}

TEST_F(IndentCppTest, SwitchCase_IndentCase) {
  buffer_->SetIndentOption("indent_case", OptionValue::FromBool(true));

  buffer_->AppendLine(L"    switch (file_format) {");
  buffer_->AppendLine(L"        case FF_WIN:");
  buffer_->AppendLine(L"            return L\"\r\n\";");
  buffer_->AppendLine(L"        case FF_UNIX:");
  buffer_->AppendLine(L"            return L\"\n\";");
  buffer_->AppendLine(L"        case FF_MAC:");
  buffer_->AppendLine(L"            return L\"\r\";");
  buffer_->AppendLine(L"        default:");
  buffer_->AppendLine(L"            return L\"\";");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
}

TEST_F(IndentCppTest, SwitchCase_DontIndentCase) {
  buffer_->SetIndentOption("indent_case", OptionValue::FromBool(false));

  buffer_->AppendLine(L"    switch (file_format) {");
  buffer_->AppendLine(L"    case FF_WIN:");
  buffer_->AppendLine(L"        return L\"\r\n\";");
  buffer_->AppendLine(L"    case FF_UNIX:");
  buffer_->AppendLine(L"        return L\"\n\";");
  buffer_->AppendLine(L"    case FF_MAC:");
  buffer_->AppendLine(L"        return L\"\r\";");
  buffer_->AppendLine(L"    default:");
  buffer_->AppendLine(L"        return L\"\";");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
}

TEST_F(IndentCppTest, Macro_OneLine) {
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"#define MAX_SIZE 256");
  buffer_->AppendLine(L"    int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, Macro_NextLineIsBrace) {
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"#define MAX_SIZE 256");
  buffer_->AppendLine(L"    {");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentCppTest, Macro_EolEscaped_IndentBody) {
  buffer_->SetIndentOption("indent_preproc_body", OptionValue::FromBool(true));

  buffer_->AppendLine(L"        int i;");
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, Macro_EolEscaped_DontIndentBody) {
  buffer_->SetIndentOption("indent_preproc_body", OptionValue::FromBool(false));

  buffer_->AppendLine(L"        int i;");
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"256");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}


TEST_F(IndentCppTest, CommentedBlockStart) {
  buffer_->AppendLine(L"        int i;  // {");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
}

TEST_F(IndentCppTest, PairedKeyInsideComment) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    // {");  // This { shouldn't affect the indent of }
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, PairedKeyInsideString) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    const char* str = \"test {\";");  // This { shouldn't affect the indent of }
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, Enum) {
  buffer_->AppendLine(L"enum WeekDay {");
  buffer_->AppendLine(L"    kSunday = 0,");
  buffer_->AppendLine(L"    kMonday,");
  buffer_->AppendLine(L"    kTuesday,");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentCppTest, Comments_1) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"    // comments");
  buffer_->AppendLine(L"    // comments");
  buffer_->AppendLine(L"    return a,");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentCppTest, Comments_2) {
  buffer_->AppendLine(L" //if (a > b) {");
  buffer_->AppendLine(L" // comments");
  buffer_->AppendLine(L" // comments");
  buffer_->AppendLine(L"return a,");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, Comments_3) {
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