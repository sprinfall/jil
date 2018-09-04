#include "editor_unittest/indent_test_base.h"

using namespace editor;

class IndentTest_Cpp : public IndentTestBase {
public:
  static void SetUpTestCase() {
    FileType ft(wxT("cpp"), wxT("C++"));
    StaticSetUp(ft);

    SetTextOptions(true, 4);
    AddQuotes();
  }

  static void TearDownTestCase() {
    StaticTearDown();
  }

  static void AddQuotes() {
    Quote* quote_comment1 = new Quote(Lex(kLexComment), L"/*", L"*/", kQuoteMultiLine);
    ft_plugin->AddQuote(quote_comment1);

    Quote* quote_comment2 = new Quote(Lex(kLexComment), L"//", L"", kQuoteEscapeEol);
    ft_plugin->AddQuote(quote_comment2);

    Quote* quote_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", kQuoteEscapeEol);
    ft_plugin->AddQuote(quote_str);

    Quote* quote_char = new Quote(Lex(kLexConstant, kLexConstantChar), L"'", L"'", 0);
    ft_plugin->AddQuote(quote_char);
  }
};

////////////////////////////////////////////////////////////////////////////////
// Test TextBuffer::GuessTabOptions with indent function.

// See editor_unittest.text_buffer_unittest.cc for testing
// TextBuffer::GuessTabOptions WITHOUT indent function.

TEST_F(IndentTest_Cpp, GuessTabOptions1) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"}");

  TabOptions tab_options(0, false);
  bool result = buffer_->GuessTabOptions(&tab_options);
  EXPECT_TRUE(result);
  EXPECT_TRUE(tab_options.expand_tab);
  EXPECT_EQ(4, tab_options.tab_stop);
}

TEST_F(IndentTest_Cpp, GuessTabOptions2) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"  return b;");
  buffer_->AppendLine(L"}");

  TabOptions tab_options(0, false);
  bool result = buffer_->GuessTabOptions(&tab_options);
  EXPECT_TRUE(result);
  EXPECT_TRUE(tab_options.expand_tab);
  EXPECT_EQ(2, tab_options.tab_stop);
}

TEST_F(IndentTest_Cpp, GuessTabOptions3) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"\treturn b;");
  buffer_->AppendLine(L"}");

  TabOptions tab_options(0, false);
  bool result = buffer_->GuessTabOptions(&tab_options);
  EXPECT_TRUE(result);
  EXPECT_FALSE(tab_options.expand_tab);
  EXPECT_EQ(0, tab_options.tab_stop);
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(IndentTest_Cpp, IsPreprocHead) {
  luabridge::LuaRef is_preproc_head = GetLuaValue(lua_state, "cpp", "isPreprocHead");
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

TEST_F(IndentTest_Cpp, IsPreprocBody) {
  luabridge::LuaRef is_preproc_body = GetLuaValue(lua_state, "cpp", "isPreprocBody");
  if (is_preproc_body.isNil() || !is_preproc_body.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");

  EXPECT_FALSE(is_preproc_body(buffer_, 2).cast<bool>());
  EXPECT_TRUE(is_preproc_body(buffer_, 3).cast<bool>());
}

TEST_F(IndentTest_Cpp, IsPreproc) {
  luabridge::LuaRef is_preproc = GetLuaValue(lua_state, "cpp", "isPreproc");
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

TEST_F(IndentTest_Cpp, GetPrevLine) {
  // getPrevLine(buffer, ln, skip_comment, skip_preproc)
  luabridge::LuaRef get_prev_line = GetLuaValue(lua_state, "cpp", "getPrevLine");
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

TEST_F(IndentTest_Cpp, GetPrevLineIndent) {
  // getPrevLineIndent(buffer, ln, skip_comment, skip_preproc)
  luabridge::LuaRef get_prev_line_indent = GetLuaValue(lua_state, "cpp", "getPrevLineIndent");
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

TEST_F(IndentTest_Cpp, GetPrevLineStartWith) {
  luabridge::LuaRef get_prev_line_start_with = GetLuaValue(lua_state, "cpp", "getPrevLineStartWith");
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

TEST_F(IndentTest_Cpp, GetBlockHead1) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"if (condition) {");

  Coord x = buffer_->Line(2)->FindLastChar(L'{', true, -1);
  EXPECT_EQ(2, get_block_head(buffer_, 2, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead2) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"if (condition)");
  buffer_->AppendLine(L"{");

  Coord ln = 3;
  Coord x = buffer_->Line(ln)->FindLastChar(L'{', true, -1);
  EXPECT_EQ(2, get_block_head(buffer_, ln, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead3) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"if (condition1 &&");
  buffer_->AppendLine(L"    condition2)");
  buffer_->AppendLine(L"{");

  Coord ln = 4;
  Coord x = buffer_->Line(ln)->FindLastChar(L'{', true, -1);

  EXPECT_EQ(2, get_block_head(buffer_, ln, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead4) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"int i;");
  buffer_->AppendLine(L"");
  buffer_->AppendLine(L"{");

  Coord ln = 4;
  Coord x = buffer_->Line(ln)->FindLastChar(L'{', true, -1);

  EXPECT_EQ(2, get_block_head(buffer_, ln, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead5) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"");
  buffer_->AppendLine(L"{");

  Coord ln = 3;
  Coord x = buffer_->Line(ln)->FindLastChar(L'{', true, -1);

  EXPECT_EQ(ln, get_block_head(buffer_, ln, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead6) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"if (condition1 &&");
  buffer_->AppendLine(L"    condition2)");
  buffer_->AppendLine(L"");
  buffer_->AppendLine(L"  // comments");
  buffer_->AppendLine(L"{");

  Coord ln = 6;
  Coord x = buffer_->Line(ln)->FindLastChar(L'{', true, -1);

  EXPECT_EQ(2, get_block_head(buffer_, ln, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead7) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"if (condition1 &&");
  buffer_->AppendLine(L"    condition2)");
  buffer_->AppendLine(L"");
  buffer_->AppendLine(L"#define MAX_PATH 256");
  buffer_->AppendLine(L"  // comments");
  buffer_->AppendLine(L"{");

  Coord ln = 7;
  Coord x = buffer_->Line(ln)->FindLastChar(L'{', true, -1);

  EXPECT_EQ(2, get_block_head(buffer_, ln, x).cast<int>());
}

TEST_F(IndentTest_Cpp, GetBlockHead8) {
  luabridge::LuaRef get_block_head = GetLuaValue(lua_state, "cpp", "getBlockHead");
  if (get_block_head.isNil() || !get_block_head.isFunction()) {
    EXPECT_TRUE(false);
    return;
  }

  buffer_->AppendLine(L"if (condition1 &&");
  buffer_->AppendLine(L"    condition2)  // comments");
  buffer_->AppendLine(L"{");

  EXPECT_EQ(2, get_block_head(buffer_, 4, 0).cast<int>());
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(IndentTest_Cpp, Namespace_Indent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(true));

  buffer_->AppendLine(L"namespace {");
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, Namespace_DontIndent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(false));

  buffer_->AppendLine(L"namespace {");
  buffer_->AppendLine(L"int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, Namespace_NewLineBrace_Indent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(true));

  buffer_->AppendLine(L"namespace");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, Namespace_NewLineBrace_DontIndent) {
  buffer_->SetIndentOption("indent_namespace", OptionValue::FromBool(false));

  buffer_->AppendLine(L"namespace");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"int i;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, SimpleBraceBlock) {
  buffer_->AppendLine(L"int a, b, c;");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    int sum = a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, SimpleBraceBlock_EmptyBody) {
  buffer_->AppendLine(L"int a, b, c;");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, FunctionDef_EmptyBody) {
  buffer_->AppendLine(L"void None() {");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
}

TEST_F(IndentTest_Cpp, FunctionDef_EmptyBody_NewLineBrace) {
  buffer_->AppendLine(L"void None()");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, FunctionDef_OneLineParams) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, FunctionDef_MultiLineParams) {
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

TEST_F(IndentTest_Cpp, FunctionDef_MultiLineParams2) {
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

TEST_F(IndentTest_Cpp, FunctionDef_OneLineParams_NewLineBrace) {
  buffer_->AppendLine(L"void add(int a, int b, int c)");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, FunctionDef_OneLineParams_NewLineBrace_Comment) {
  buffer_->AppendLine(L"void add(int a, int b, int c)  // comment");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, FunctionDef_MultiLineParams_NewLineBrace) {
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

TEST_F(IndentTest_Cpp, FunctionDef_MultiLineParams_NewLineBrace_Comment) {
  buffer_->AppendLine(L"void add(int a,");
  buffer_->AppendLine(L"         int b,");
  buffer_->AppendLine(L"         int c)  // comment");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentTest_Cpp, FunctionDef_DecoratedMethod) {
  buffer_->AppendLine(L"    int GetSomething() const {");
  buffer_->AppendLine(L"        return something;");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, FunctionDef_DecoratedMethod_NewLineBrace) {
  buffer_->AppendLine(L"    int GetSomething() const");
  buffer_->AppendLine(L"    {");
  buffer_->AppendLine(L"        return something;");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, FunctionDef_DecoratedMethod_MultiLineParams) {
  buffer_->AppendLine(L"    virtual void CalcSomething(int param1,");
  buffer_->AppendLine(L"                               int param2) override {");
  buffer_->AppendLine(L"        return something;");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, FunctionCall_StringParenthesis) {
  buffer_->AppendLine(L"TextPoint p = buffer->UnpairedLeftKey(TextPoint(j, prev_ln),");
  buffer_->AppendLine(L"                                      L'(',");
  buffer_->AppendLine(L"                                      L')');");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, If_OneLineConditions) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, If_NoBrace) {
  buffer_->AppendLine(L"if (a > b)");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, If_NoBrace_WithElse) {
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

TEST_F(IndentTest_Cpp, If_NoBrace_OneLine) {
  buffer_->AppendLine(L"if (a > b) return b;");
  buffer_->AppendLine(L"else return a;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, For_NoBrace) {
  buffer_->AppendLine(L"for (int i = 0; i < count; ++i)");
  buffer_->AppendLine(L"    sum += i;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, For_MultiLine) {
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

TEST_F(IndentTest_Cpp, While_NoBrace) {
  buffer_->AppendLine(L"while (i < count)");
  buffer_->AppendLine(L"    sum += i;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, While_NoBrace_OnLine) {
  buffer_->AppendLine(L"while (i < count) sum += i;");
  buffer_->AppendLine(L"int i;");

  ASSERT_LINE(3);
}

TEST_F(IndentTest_Cpp, Class) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, Class_PublicAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");

  ASSERT_LINE(3);
}

TEST_F(IndentTest_Cpp, Class_ProtectedAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    protected:");

  ASSERT_LINE(3);
}

TEST_F(IndentTest_Cpp, Class_PrivateAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    private:");

  ASSERT_LINE(3);
}

TEST_F(IndentTest_Cpp, Class_EmptyAccessors) {
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

TEST_F(IndentTest_Cpp, Class_Accessors) {
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

TEST_F(IndentTest_Cpp, Class_Accessors_Comment) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    /*");
  buffer_->AppendLine(L"        class B {");
  buffer_->AppendLine(L"    */");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"        ~A();");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"        A();");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
  ASSERT_LINE(10);
  ASSERT_LINE(11);
  ASSERT_LINE(12);
}

TEST_F(IndentTest_Cpp, Struct_EmptyAccessors) {
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

TEST_F(IndentTest_Cpp, Struct_Accessors) {
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

TEST_F(IndentTest_Cpp, Class_Accessors_Negative1) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public :");
  buffer_->AppendLine(L"        ~A();");
  buffer_->AppendLine(L"    protected  :");
  buffer_->AppendLine(L"        A();");
  buffer_->AppendLine(L"    private\t:");
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

TEST_F(IndentTest_Cpp, SwitchCase_IndentCase) {
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

TEST_F(IndentTest_Cpp, SwitchCase_DontIndentCase) {
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

TEST_F(IndentTest_Cpp, SwitchCase_Comment) {
  buffer_->SetIndentOption("indent_case", OptionValue::FromBool(false));

  buffer_->AppendLine(L"    switch (file_format) {");
  buffer_->AppendLine(L"    /* ");
  buffer_->AppendLine(L"        switch ");
  buffer_->AppendLine(L"    */ ");
  buffer_->AppendLine(L"    case FF_WIN:");
  buffer_->AppendLine(L"        return L\"\r\n\";");
  buffer_->AppendLine(L"    case FF_UNIX:");
  buffer_->AppendLine(L"        return L\"\n\";");
  buffer_->AppendLine(L"    case FF_MAC:");
  buffer_->AppendLine(L"        return L\"\r\";");
  buffer_->AppendLine(L"    default:");
  buffer_->AppendLine(L"        return L\"\";");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
  ASSERT_LINE(10);
  ASSERT_LINE(11);
  ASSERT_LINE(12);
}

TEST_F(IndentTest_Cpp, Macro_OneLine) {
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"#define MAX_SIZE 256");
  buffer_->AppendLine(L"    int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, Macro_NextLineIsBrace) {
  buffer_->AppendLine(L"    int i;");
  buffer_->AppendLine(L"#define MAX_SIZE 256");
  buffer_->AppendLine(L"    {");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Cpp, Macro_EolEscaped_IndentBody) {
  buffer_->SetIndentOption("indent_preproc_body", OptionValue::FromBool(true));

  buffer_->AppendLine(L"        int i;");
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, Macro_EolEscaped_DontIndentBody) {
  buffer_->SetIndentOption("indent_preproc_body", OptionValue::FromBool(false));

  buffer_->AppendLine(L"        int i;");
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"256");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}


TEST_F(IndentTest_Cpp, CommentedBlockStart) {
  buffer_->AppendLine(L"        int i;  // {");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
}

TEST_F(IndentTest_Cpp, PairedKeyInsideComment) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    // {");  // This { shouldn't affect the indent of }
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, PairedKeyInsideString) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    const char* str = \"test {\";");  // This { shouldn't affect the indent of }
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, Enum) {
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

TEST_F(IndentTest_Cpp, Comments_1) {
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

TEST_F(IndentTest_Cpp, Comments_2) {
  buffer_->AppendLine(L" //if (a > b) {");
  buffer_->AppendLine(L" // comments");
  buffer_->AppendLine(L" // comments");
  buffer_->AppendLine(L"return a,");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cpp, Comments_3) {
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