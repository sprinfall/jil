#include <memory>
#include "editor/ft_plugin.h"
#include "editor/indent_cfg.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"
#include "gtest/gtest.h"

using namespace jil::editor;

typedef std::auto_ptr<TextBuffer> TextBufferPtr;

static const Encoding kEncoding = EncodingFromName(ENCODING_NAME_ISO_8859_1);

class IndentCfgTest : public testing::Test {
protected:
  virtual void SetUp() {
    ft_plugin_ = new FtPlugin(FileType(wxT("cfg"), wxT("Config")));

    TextOptions text_options;
    text_options.expand_tab = true;
    text_options.tab_stop = 4;
    text_options.shift_width = 4;
    text_options.delimiters = L"!@#%^$&*()+-=\\|/?[]{}<>,.;:'\"`~";
    ft_plugin_->set_text_options(text_options);

    Quote* quote_comment1 = new Quote(Lex(kLexComment), L"/*", L"*/", kQuoteMultiLine);
    ft_plugin_->AddQuote(quote_comment1);

    Quote* quote_comment2 = new Quote(Lex(kLexComment), L"//", L"", 0);
    ft_plugin_->AddQuote(quote_comment2);

    Quote* quote_comment3 = new Quote(Lex(kLexComment), L"#", L"", 0);
    ft_plugin_->AddQuote(quote_comment3);

    Quote* quote_str = new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", 0);
    ft_plugin_->AddQuote(quote_str);

    buffer_ = TextBuffer::Create(0, ft_plugin_, kEncoding);

    indent_cfg_ = IndentCfg;
  }

  virtual void TearDown() {
    delete buffer_;
    delete ft_plugin_;
  }

  FtPlugin* ft_plugin_;
  TextBuffer* buffer_;
  IndentFunc indent_cfg_;
};

#define ASSERT_LINE(ln)\
  EXPECT_EQ(buffer_->GetIndent(ln), indent_cfg_(buffer_, ln));

////////////////////////////////////////////////////////////////////////////////

TEST_F(IndentCfgTest, Group) {
  buffer_->AppendLine(L"{");
  buffer_->AppendLine(L"    id = \"identifier\";");
  buffer_->AppendLine(L"    next = \":\";");
  buffer_->AppendLine(L"}");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCfgTest, NamedGroup) {
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

TEST_F(IndentCfgTest, Array) {
  buffer_->AppendLine(L"indent_keys = [");
  buffer_->AppendLine(L"    \"{\",");
  buffer_->AppendLine(L"    \"}\"");
  buffer_->AppendLine(L"];");

  ASSERT_LINE(3);
  ASSERT_LINE(4);
  ASSERT_LINE(5);
}

TEST_F(IndentCfgTest, List) {
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

TEST_F(IndentCfgTest, AssignOperator1) {
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

TEST_F(IndentCfgTest, AssignOperator2) {
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
