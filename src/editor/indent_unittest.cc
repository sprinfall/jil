#include <memory>
#include "editor/ft_plugin.h"
#include "editor/indent.h"
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
    ft_plugin_ = new FtPlugin(FileType(wxT("cpp"), wxT("C++")));

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

    buffer_ = TextBuffer::Create(0, ft_plugin_, kEncoding);

    indent_cpp_ = IndentCpp;
  }

  virtual void TearDown() {
    delete buffer_;
    delete ft_plugin_;
  }

  FtPlugin* ft_plugin_;
  TextBuffer* buffer_;
  IndentFunc indent_cpp_;
};

#define ASSERT_LINE(ln)\
  EXPECT_EQ(buffer_->GetIndent(ln), indent_cpp_(buffer_, ln));

////////////////////////////////////////////////////////////////////////////////
// Test helper function:
//   bool cpp::IsLineMacro(const TextBuffer* buffer, Coord ln);

TEST_F(IndentCppTest, IsLineMacro1) {
  buffer_->AppendLine(L"#define MAX_SIZE 256");

  EXPECT_TRUE(cpp::IsLineMacro(buffer_, 2));
}

TEST_F(IndentCppTest, IsLineMacro2) {
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");

  EXPECT_TRUE(cpp::IsLineMacro(buffer_, 2));
  EXPECT_TRUE(cpp::IsLineMacro(buffer_, 3));
}

TEST_F(IndentCppTest, IsLineMacro3) {
  buffer_->AppendLine(L"#define MAX_SIZE 256 \\");
  buffer_->AppendLine(L"");
  buffer_->AppendLine(L"   int i;");

  EXPECT_TRUE(cpp::IsLineMacro(buffer_, 2));
  EXPECT_TRUE(cpp::IsLineMacro(buffer_, 3));
  EXPECT_FALSE(cpp::IsLineMacro(buffer_, 4));
}

////////////////////////////////////////////////////////////////////////////////

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
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentCppTest, FunctionDef_MultiLineParams2) {
  buffer_->AppendLine(L"\t void add(int a,");  // Note the '\t'.
  buffer_->AppendLine(L"              int b,");
  buffer_->AppendLine(L"              int c) {");
  buffer_->AppendLine(L"         return a + b + c;");
  buffer_->AppendLine(L"     }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
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

TEST_F(IndentCppTest, If_OneLineConditions) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
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
  //buffer_->AppendLine(L"int i;");

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

TEST_F(IndentCppTest, SwitchCase) {
  // TODO: Set indent option
  //buffer_->SetIndentOption("indent_case", false);

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

//TEST_F(IndentCppTest, Macro_OneLine) {
//  buffer_->AppendLine(L"    int i;");
//  buffer_->AppendLine(L"#define MAX_SIZE 256");
//  buffer_->AppendLine(L"    int j;");
//
//  ASSERT_LINE(3);
//  ASSERT_LINE(4);
//}

//TEST_F(IndentCppTest, Macro_EolEscaped) {
//  buffer_->AppendLine(L"        int i;");
//  buffer_->AppendLine(L"#define MAX_SIZE \\");
//  buffer_->AppendLine(L"    256");
//  buffer_->AppendLine(L"        int j;");
//
//  Assert(3);
//}
//
//TEST_F(IndentCppTest, Macro_EolEscaped2) {
//  buffer_->AppendLine(L"        int i;");
//  buffer_->AppendLine(L"#define MAX_SIZE 256 \\");
//  buffer_->AppendLine(L"");
//  buffer_->AppendLine(L"        int j;");
//
//  Assert(3);
//}

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

  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCppTest, PairedKeyInsideString) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    const char* str = \"test {\";");  // This { shouldn't affect the indent of }
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(4);
  ASSERT_LINE(5);
}