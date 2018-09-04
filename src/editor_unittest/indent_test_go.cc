#include "editor_unittest/indent_test_base.h"

using namespace editor;

class IndentTest_Go : public IndentTestBase {
public:
  static void SetUpTestCase() {
    FileType ft(wxT("go"), wxT("Go"));
    StaticSetUp(ft);

    SetTextOptions(true, 4);
    AddQuotes();
  }

  static void TearDownTestCase() {
    StaticTearDown();
  }

protected:
  static void AddQuotes() {
    Quote* quote_comment1 = new Quote(Lex(kLexComment), L"/*", L"*/", kQuoteMultiLine);
    ft_plugin->AddQuote(quote_comment1);

    Quote* quote_comment2 = new Quote(Lex(kLexComment), L"//", L"", 0);
    ft_plugin->AddQuote(quote_comment2);

    Quote* quote_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", kQuoteEscapeEol);
    ft_plugin->AddQuote(quote_str);

    Quote* quote_raw_str = new Quote(Lex(kLexConstant, kLexConstantString), L"`", L"`", kQuoteMultiLine);
    ft_plugin->AddQuote(quote_raw_str);

    Quote* quote_char = new Quote(Lex(kLexConstant, kLexConstantChar), L"'", L"'", 0);
    ft_plugin->AddQuote(quote_char);
  }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(IndentTest_Go, Import) {
  buffer_->AppendLine(L"import (");
  buffer_->AppendLine(L"    \"fmt\"");
  buffer_->AppendLine(L"    \"math / rand\"");
  buffer_->AppendLine(L")");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Go, Function) {
  buffer_->AppendLine(L"func add(x int, y int) int {");
  buffer_->AppendLine(L"    return x + y");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Go, Function_MultiResults) {
  buffer_->AppendLine(L"func swap(x, y string) (string, string) {");
  buffer_->AppendLine(L"    return y, x");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Go, Function_NamedResults) {
  buffer_->AppendLine(L"func split(sum int) (x, y int) {");
  buffer_->AppendLine(L"    x = sum * 4 / 9");
  buffer_->AppendLine(L"    y = sum - x");
  buffer_->AppendLine(L"    return");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentTest_Go, Var) {
  buffer_->AppendLine(L"var (");
  buffer_->AppendLine(L"    v1 = Vertex{ 1, 2 }");
  buffer_->AppendLine(L"    v2 = Vertex{ X: 1 }");
  buffer_->AppendLine(L"    v3 = Vertex{}");
  buffer_->AppendLine(L"    p = &Vertex{ 1, 2 }");
  buffer_->AppendLine(L")");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentTest_Go, Const) {
  buffer_->AppendLine(L"const (");
  buffer_->AppendLine(L"    Big = 1 << 100");
  buffer_->AppendLine(L"    Small = Big >> 99");
  buffer_->AppendLine(L")");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Go, Struct) {
  buffer_->AppendLine(L"type Vertex struct {");
  buffer_->AppendLine(L"    X int");
  buffer_->AppendLine(L"    Y int");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Go, For) {
  buffer_->AppendLine(L"sum: = 0");
  buffer_->AppendLine(L"for i : = 0; i < 10; i++ {");
  buffer_->AppendLine(L"    sum += i");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Go, For_Empty) {
  buffer_->AppendLine(L"sum: = 0");
  buffer_->AppendLine(L"for {");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_Go, If) {
  buffer_->AppendLine(L"func sqrt(x float64) string {");
  buffer_->AppendLine(L"    if x < 0 {");
  buffer_->AppendLine(L"        return sqrt(-x) + \"i\"");
  buffer_->AppendLine(L"    }");
  buffer_->AppendLine(L"    return fmt.Sprint(math.Sqrt(x))");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentTest_Go, IfElse) {
  buffer_->AppendLine(L"func pow(x, n, lim float64) float64 {");
  buffer_->AppendLine(L"    if v : = math.Pow(x, n); v < lim {");
  buffer_->AppendLine(L"        return v");
  buffer_->AppendLine(L"    } else {");
  buffer_->AppendLine(L"        fmt.Printf(\"%g >= %g\n\", v, lim)");
  buffer_->AppendLine(L"    }");
  buffer_->AppendLine(L"    // can't use v here, though");
  buffer_->AppendLine(L"    return lim");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
  ASSERT_LINE(10);
}

TEST_F(IndentTest_Go, Switch) {
  buffer_->AppendLine(L"    switch os : = runtime.GOOS; os {");
  buffer_->AppendLine(L"    case \"darwin\":");
  buffer_->AppendLine(L"        fmt.Println(\"OS X.\")");
  buffer_->AppendLine(L"    case \"linux\":");
  buffer_->AppendLine(L"        fmt.Println(\"Linux.\")");
  buffer_->AppendLine(L"    default:");
  buffer_->AppendLine(L"        // freebsd, openbsd,");
  buffer_->AppendLine(L"        // plan9, windows...");
  buffer_->AppendLine(L"        fmt.Printf(\"%s.\", os)");
  buffer_->AppendLine(L"    }");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
  ASSERT_LINE(8);
  ASSERT_LINE(9);
  ASSERT_LINE(10);
  ASSERT_LINE(11);
}

// Comma
//func printSlice(s string, x[]int) {
//\tfmt.Printf("%s len=%d cap=%d %v\n",
//\t\ts, len(x), cap(x), x)
//}