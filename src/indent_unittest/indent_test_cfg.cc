#include "indent_unittest/indent_test_base.h"

using namespace editor;

class IndentTest_Cfg : public IndentTestBase {
public:
  static void SetUpTestCase() {
    FileType ft(wxT("cfg"), wxT("Config"));
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

    Quote* quote_comment2 = new Quote(Lex(kLexComment), L"//", L"", 0);
    ft_plugin->AddQuote(quote_comment2);

    Quote* quote_comment3 = new Quote(Lex(kLexComment), L"#", L"", 0);
    ft_plugin->AddQuote(quote_comment3);

    Quote* quote_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", 0);
    ft_plugin->AddQuote(quote_str);
  }
};

////////////////////////////////////////////////////////////////////////////////

TEST_F(IndentTest_Cfg, Group) {
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    id = \"identifier\";");
  buffer_->AppendLine(L"    next = \":\";");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cfg, NamedGroup) {
  buffer_->AppendLine(L"indent = {");
  buffer_->AppendLine(L"    indent_namespace = true;");
  buffer_->AppendLine(L"    indent_case = false;");
  buffer_->AppendLine(L"    indent_macro_body = true;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentTest_Cfg, Array) {
  buffer_->AppendLine(L"indent_keys = [");
  buffer_->AppendLine(L"    \"{\",");
  buffer_->AppendLine(L"    \"}\"");
  buffer_->AppendLine(L"];");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentTest_Cfg, List) {
  buffer_->AppendLine(L"rect = (");
  buffer_->AppendLine(L"    0,");
  buffer_->AppendLine(L"    0,");
  buffer_->AppendLine(L"    0,");
  buffer_->AppendLine(L"    0");
  buffer_->AppendLine(L")");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}

TEST_F(IndentTest_Cfg, AssignOperator1) {
  buffer_->AppendLine(L"indent_keys = ");
  buffer_->AppendLine(L"[");
  buffer_->AppendLine(L"    \"{\",");
  buffer_->AppendLine(L"    \"}\"");
  buffer_->AppendLine(L"];");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
}

TEST_F(IndentTest_Cfg, AssignOperator2) {
  buffer_->AppendLine(L"indent = ");
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    indent_namespace = true;");
  buffer_->AppendLine(L"    indent_case = false;");
  buffer_->AppendLine(L"    indent_macro_body = true;");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
  ASSERT_LINE(6);
  ASSERT_LINE(7);
}
