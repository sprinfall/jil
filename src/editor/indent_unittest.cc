#include <memory>
#include "editor/ft_plugin.h"
#include "editor/indent.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"
#include "gtest/gtest.h"

using namespace jil::editor;

class CppIndentTest : public testing::Test {
protected:
  virtual void SetUp() {
    ft_plugin_ = new FtPlugin(FileType(wxT("cpp"), wxT("C++")));
    ft_plugin_->options().expand_tab = true;
    ft_plugin_->options().tab_stop = 4;
    ft_plugin_->options().shift_width = 4;

    Encoding encoding = EncodingFromName(ENCODING_NAME_ISO_8859_1);

    buffer_ = TextBuffer::Create(ft_plugin_, encoding);

    indent_cpp_ = new IndentCpp;
    indent_cpp_->SetBuffer(buffer_);
  }

  virtual void TearDown() {
    delete indent_cpp_;
    delete buffer_;
    delete ft_plugin_;
  }

  void Assert(Coord first, Coord last = kInvalidCoord) {
    if (last == kInvalidCoord) {
      last = buffer_->LineCount();
    }

    for (Coord ln = first; ln <= last; ++ln) {
      EXPECT_EQ(buffer_->GetIndent(ln), indent_cpp_->Indent(ln));
    }
  }

  FtPlugin* ft_plugin_;
  TextBuffer* buffer_;
  IndentCpp* indent_cpp_;
};

TEST_F(CppIndentTest, SimpleBraceBlock) {
  buffer_->AppendLine(L"int a, b, c;");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    int sum = a + b + c;");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, SimpleBraceBlock_EmptyBody) {
  buffer_->AppendLine(L"int a, b, c;");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_EmptyBody) {
  buffer_->AppendLine(L"void None() {");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_EmptyBody_NewLineBrace) {
  buffer_->AppendLine(L"void None()");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_OneLineParams) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_MultiLineParams) {
  buffer_->AppendLine(L"void add(int a,");
  buffer_->AppendLine(L"         int b,");
  buffer_->AppendLine(L"         int c) {");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_MultiLineParams2) {
  buffer_->AppendLine(L"\t void add(int a,");  // Note the '\t'.
  buffer_->AppendLine(L"              int b,");
  buffer_->AppendLine(L"              int c) {");
  buffer_->AppendLine(L"         return a + b + c;");
  buffer_->AppendLine(L"     }");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_OneLineParams_NewLineBrace) {
  buffer_->AppendLine(L"void add(int a, int b, int c)");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, FunctionDef_MultiLineParams_NewLineBrace) {
  buffer_->AppendLine(L"void add(int a,");
  buffer_->AppendLine(L"         int b,");
  buffer_->AppendLine(L"         int c)");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  Assert(3);
}

//TEST_F(CppIndentTest, FunctionCall_StringParenthesis) {;
//  buffer_->AppendLine(L"TextPoint p = buffer->UnpairedLeftKey(TextPoint(j, prev_ln),");
//  buffer_->AppendLine(L"                                      L'(',");
//  buffer_->AppendLine(L"                                      L')');");
//
//  Assert(3);
//}

TEST_F(CppIndentTest, If_NoBrace) {
  buffer_->AppendLine(L"if (a > b)");
  buffer_->AppendLine(L"    return b;");
  //buffer_->AppendLine(L"else");
  //buffer_->AppendLine(L"    return a;");
  //buffer_->AppendLine(L"int i;");

  Assert(3);
}

TEST_F(CppIndentTest, If_OneLineConditions) {
  buffer_->AppendLine(L"if (a > b) {");
  buffer_->AppendLine(L"    return b;");
  buffer_->AppendLine(L"}");

  Assert(3);
}

TEST_F(CppIndentTest, If_NoBrace_OneLine) {
  buffer_->AppendLine(L"if (a > b) return b;");
  buffer_->AppendLine(L"else return a;");
  buffer_->AppendLine(L"int i;");

  Assert(3);
}

TEST_F(CppIndentTest, For_NoBrace) {
  buffer_->AppendLine(L"for (int i = 0; i < count; ++i)");
  buffer_->AppendLine(L"    sum += i;");
  //buffer_->AppendLine(L"int i;");

  Assert(3);
}

TEST_F(CppIndentTest, Class) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  Assert(3);
}

TEST_F(CppIndentTest, Class_PublicAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");

  Assert(3);
}

TEST_F(CppIndentTest, Class_ProtectedAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    protected:");

  Assert(3);
}

TEST_F(CppIndentTest, Class_PrivateAccessor) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    private:");

  Assert(3);
}

TEST_F(CppIndentTest, Class_EmptyAccessors) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"    };");

  Assert(3);
}

TEST_F(CppIndentTest, Class_Accessors) {
  buffer_->AppendLine(L"    class A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"        ~A();");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"        A();");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  Assert(3);
}

TEST_F(CppIndentTest, Struct_EmptyAccessors) {
  buffer_->AppendLine(L"    struct A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"    };");

  Assert(3);
}

TEST_F(CppIndentTest, Struct_Accessors) {
  buffer_->AppendLine(L"    struct A {");
  buffer_->AppendLine(L"    public:");
  buffer_->AppendLine(L"        ~A();");
  buffer_->AppendLine(L"    protected:");
  buffer_->AppendLine(L"        A();");
  buffer_->AppendLine(L"    private:");
  buffer_->AppendLine(L"        int count_;");
  buffer_->AppendLine(L"    };");

  Assert(3);
}

TEST_F(CppIndentTest, SwitchCase) {
  indent_cpp_->set_indent_case(false);

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

  Assert(3, 4);
}
