#include "indent_unittest/indent_test_base.h"

using namespace jil::editor;

class IndentTest_Go : public IndentTestBase {
public:
  static void SetUpTestCase() {
    FileType ft(wxT("go"), wxT("Go"));
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

    Quote* quote_comment2 = new Quote(Lex(kLexComment), L"//", L"", 0);
    ft_plugin->AddQuote(quote_comment2);

    Quote* quote_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", kQuoteEscapeEol);
    ft_plugin->AddQuote(quote_str);

    Quote* quote_raw_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\`", L"\`", kQuoteMultiLine);
    ft_plugin->AddQuote(quote_raw_str);

    Quote* quote_char = new Quote(Lex(kLexConstant, kLexConstantChar), L"'", L"'", 0);
    ft_plugin->AddQuote(quote_char);
  }
};

////////////////////////////////////////////////////////////////////////////////

// TODO