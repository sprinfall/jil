#include <memory>
#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"
#include "gtest/gtest.h"

using namespace editor;

static const FileType kFtTxt("txt", "Text");
static const Encoding kEncoding = GetEncodingById(ENCODING_ISO_8859_1);

#if JIL_FIND_REGEX_ACROSS_LINES

TEST(TextBuffer, CharIterator) {
  FtPlugin ft_plugin(kFtTxt);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  std::wstring text = L"1st line\n2nd line\n3rd line\n4th line";
  buffer.SetText(text);

  size_t i = 0;
  TextBuffer::CharIterator ci(buffer.CharBegin());
  for (; ci != buffer.CharEnd(); ++ci, ++i) {
    EXPECT_EQ(text[i], *ci);
  }

  EXPECT_EQ(TextPoint(buffer.LineLength(4), 4),
            buffer.PointFromCharIterator(ci));

  --ci;
  EXPECT_EQ(TextPoint(buffer.LineLength(4) - 1, 4),
            buffer.PointFromCharIterator(ci));
}

#endif  // JIL_FIND_REGEX_ACROSS_LINES

////////////////////////////////////////////////////////////////////////////////

TEST(TextBuffer, SetText_EmptyLines) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  std::wstring text;
  buffer.SetText(text);
  EXPECT_EQ(1, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));

  text = L"\r";
  buffer.SetText(text);
  EXPECT_EQ(2, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(2)->IsEmpty(true));

  text = L"\n";
  buffer.SetText(text);
  EXPECT_EQ(2, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(2)->IsEmpty(true));

  text = L"\r\n";
  buffer.SetText(text);
  EXPECT_EQ(2, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(2)->IsEmpty(true));

  text = L"\r\r";
  buffer.SetText(text);
  EXPECT_EQ(3, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(2)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(3)->IsEmpty(true));

  text = L"\n\n";
  buffer.SetText(text);
  EXPECT_EQ(3, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(2)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(3)->IsEmpty(true));

  text = L"\r\n\r\n";
  buffer.SetText(text);
  EXPECT_EQ(3, buffer.LineCount());
  EXPECT_TRUE(buffer.Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(2)->IsEmpty(true));
  EXPECT_TRUE(buffer.Line(3)->IsEmpty(true));
}

TEST(TextBuffer, SetText) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  std::wstring text = L"abc";
  buffer.SetText(text);
  EXPECT_EQ(1, buffer.LineCount());
  EXPECT_EQ(L"abc", buffer.LineData(1));

  text = L"abc\nde";
  buffer.SetText(text);
  EXPECT_EQ(2, buffer.LineCount());
  EXPECT_EQ(L"abc", buffer.LineData(1));
  EXPECT_EQ(L"de", buffer.LineData(2));

  text = L"abc\rde";
  buffer.SetText(text);
  EXPECT_EQ(2, buffer.LineCount());
  EXPECT_EQ(L"abc", buffer.LineData(1));
  EXPECT_EQ(L"de", buffer.LineData(2));

  text = L"abc\r\nde";
  buffer.SetText(text);
  EXPECT_EQ(2, buffer.LineCount());
  EXPECT_EQ(L"abc", buffer.LineData(1));
  EXPECT_EQ(L"de", buffer.LineData(2));

  text = L"abc\r\nde\r\n";
  buffer.SetText(text);
  EXPECT_EQ(3, buffer.LineCount());
  EXPECT_EQ(L"abc", buffer.LineData(1));
  EXPECT_EQ(L"de", buffer.LineData(2));
  EXPECT_EQ(L"", buffer.LineData(3));
}

////////////////////////////////////////////////////////////////////////////////

TEST(TextBuffer, PrevNonEmptyLine) {
  FtPlugin ft_plugin(FileType("cpp", "C++"), NULL);
  ft_plugin.AddQuote(new Quote(Lex(kLexComment), L"//", L"", kQuoteEscapeEol));
  ft_plugin.AddQuote(new Quote(Lex(kLexComment), L"/*", L"*/", kQuoteMultiLine));
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"int i;");  // Line 2
  buffer.AppendLine(L"while(true) {} // nothing");
  buffer.AppendLine(L"// line comments");
  buffer.AppendLine(L"  \t// line comments");
  buffer.AppendLine(L"");
  buffer.AppendLine(L" /* test */ /* test */  ");
  buffer.AppendLine(L"/* block");
  buffer.AppendLine(L"   comments */");
  buffer.AppendLine(L"  \t");
  buffer.AppendLine(L"int j;");  // Line 11

  EXPECT_EQ(3, buffer.PrevNonEmptyLine(11, true));
  EXPECT_EQ(9, buffer.PrevNonEmptyLine(11, false));
}

////////////////////////////////////////////////////////////////////////////////

// Test TextBuffer::GuessTabOptions WITHOUT indent function.

TEST(TextBuffer, GuessTabOptions1) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"if (a > b) {");
  buffer.AppendLine(L"    return b;");
  buffer.AppendLine(L"}");

  TabOptions tab_options(0, false);
  bool result = buffer.GuessTabOptions(&tab_options);

  EXPECT_TRUE(result);
  EXPECT_TRUE(tab_options.expand_tab);
  EXPECT_EQ(4, tab_options.tab_stop);
}

TEST(TextBuffer, GuessTabOptions2) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"if (a > b) {");
  buffer.AppendLine(L"  return b;");
  buffer.AppendLine(L"}");

  TabOptions tab_options(0, false);
  bool result = buffer.GuessTabOptions(&tab_options);

  EXPECT_TRUE(result);
  EXPECT_TRUE(tab_options.expand_tab);
  EXPECT_EQ(2, tab_options.tab_stop);
}

TEST(TextBuffer, GuessTabOptions3) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"if (a > b) {");
  buffer.AppendLine(L"\treturn b;");
  buffer.AppendLine(L"}");

  TabOptions tab_options(0, false);
  bool result = buffer.GuessTabOptions(&tab_options);

  EXPECT_TRUE(result);
  EXPECT_FALSE(tab_options.expand_tab);
  EXPECT_EQ(0, tab_options.tab_stop);
}

////////////////////////////////////////////////////////////////////////////////

// Helper text listener to test notify.
class TestTextListener : public TextListener {
public:
  TestTextListener() {
  }

  virtual ~TestTextListener() {
  }

  virtual void OnBufferLineChange(LineChangeType type,
                                  const LineChangeData& data) {
    line_change_type_ = type;
    line_change_data_ = data;
  }

  virtual void OnBufferChange(ChangeType type) {
  }

  LineChangeType line_change_type() const {
    return line_change_type_;
  }
  const LineChangeData& line_change_data() const {
    return line_change_data_;
  }

private:
  // Last change type and data.
  LineChangeType line_change_type_;
  LineChangeData line_change_data_;
};

// Test the notifications of different text changes.
TEST(TextBuffer, Notify) {
  FtPlugin ft_plugin(kFtTxt, NULL);

  TestTextListener listener;

  TextBuffer buffer(0, &ft_plugin, kEncoding);
  buffer.SetText(L"line 1");
  buffer.AttachListener(&listener);

  buffer.AppendLine(L"line 2");

  EXPECT_EQ(kLineAdded, listener.line_change_type());

  const LineChangeData& line_change_data = listener.line_change_data();
  EXPECT_EQ(2, line_change_data.first());
  EXPECT_EQ(2, line_change_data.last());
}

////////////////////////////////////////////////////////////////////////////////

TEST(TextBuffer, UnpairedLeftKey) {
  FtPlugin ft_plugin(FileType("cpp", "C++"), NULL);
  ft_plugin.AddQuote(new Quote(Lex(kLexComment), L"//", L"", kQuoteEscapeEol));
  ft_plugin.AddQuote(new Quote(Lex(kLexComment), L"/*", L"*/", kQuoteMultiLine));
  ft_plugin.AddQuote(new Quote(Lex(kLexConstant, kLexConstantString), L"\"", L"\"", kQuoteEscapeEol));

  TextBuffer buffer(0, &ft_plugin, kEncoding);
  buffer.AppendLine(L"(    ");  // Line 2

  TextPoint point(buffer.LineLength(2), 2);
  EXPECT_EQ(TextPoint(0, 2), buffer.UnpairedLeftKey(point, L'(', L')', true));
}

TEST(TextBuffer, FindStringAll_Regex1) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"test test");  // L2
  buffer.AppendLine(L"");
  buffer.AppendLine(L" test");

  std::list<TextRange> result_ranges;
  buffer.FindStringAll(L"test", buffer.range(), true, true, false, &result_ranges);

  EXPECT_EQ(3, result_ranges.size());

  std::list<TextRange>::iterator it = result_ranges.begin();
  EXPECT_EQ(TextRange(TextPoint(0, 2), TextPoint(4, 2)), *it++);
  EXPECT_EQ(TextRange(TextPoint(5, 2), TextPoint(9, 2)), *it++);
  EXPECT_EQ(TextRange(TextPoint(1, 4), TextPoint(5, 4)), *it++);
}

TEST(TextBuffer, FindStringAll_Regex2) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"test test");  // L2
  buffer.AppendLine(L"");
  buffer.AppendLine(L" test");

  std::list<TextRange> result_ranges;
  buffer.FindStringAll(L"t.st", buffer.range(), true, true, false, &result_ranges);

  EXPECT_EQ(3, result_ranges.size());

  std::list<TextRange>::iterator it = result_ranges.begin();
  EXPECT_EQ(TextRange(TextPoint(0, 2), TextPoint(4, 2)), *it++);
  EXPECT_EQ(TextRange(TextPoint(5, 2), TextPoint(9, 2)), *it++);
  EXPECT_EQ(TextRange(TextPoint(1, 4), TextPoint(5, 4)), *it++);
}

TEST(TextBuffer, FindStringAll_Regex3) {
  FtPlugin ft_plugin(kFtTxt, NULL);
  TextBuffer buffer(0, &ft_plugin, kEncoding);

  buffer.AppendLine(L"test test");  // L2
  buffer.AppendLine(L"");
  buffer.AppendLine(L" test");

  std::list<TextRange> result_ranges;
  TextRange range(TextPoint(0, 4), TextPoint(kInvCoord, 4));
  buffer.FindStringAll(L"t.{2}t", range, true, true, false, &result_ranges);

  EXPECT_EQ(1, result_ranges.size());

  EXPECT_EQ(TextRange(TextPoint(1, 4), TextPoint(5, 4)), result_ranges.front());
}
