#include "indent_unittest/indent_test_base.h"

using namespace jil::editor;

class IndentTest_C : public IndentTestBase {
public:
  static void SetUpTestCase() {
    FileType ft(wxT("c"), wxT("C"));
    StaticSetUp(ft);

    SetTextOptions(true, 4, 4);
    AddQuotes();
  }

  static void TearDownTestCase() {
    StaticTearDown();
  }

protected:
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

// Don't have to provide complete cases since it reuses C++'s indent function.

TEST_F(IndentTest_C, FunctionDef_OneLineParams) {
  buffer_->AppendLine(L"void add(int a, int b, int c) {");
  buffer_->AppendLine(L"    return a + b + c;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
}

TEST_F(IndentTest_C, FunctionDef_MultiLineParams) {
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

TEST_F(IndentTest_C, Macro_EolEscaped_IndentBody) {
  buffer_->SetIndentOption("indent_preproc_body", OptionValue::FromBool(true));

  buffer_->AppendLine(L"        int i;");
  buffer_->AppendLine(L"#define MAX_SIZE \\");
  buffer_->AppendLine(L"    256");
  buffer_->AppendLine(L"        int j;");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_C, Comments) {
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
