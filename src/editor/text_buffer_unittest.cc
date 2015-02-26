#include <memory>
#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"
#include "gtest/gtest.h"

using namespace jil::editor;

typedef std::auto_ptr<TextBuffer> TextBufferPtr;

static const FileType kFtTxt("txt", "Text");
static const Encoding kEncoding = EncodingFromName(ENCODING_NAME_ISO_8859_1);

TEST(TextBuffer, CharIterator) {
  FtPlugin ft_plugin(kFtTxt);

  TextBufferPtr buffer(TextBuffer::Create(&ft_plugin, kEncoding));

  std::wstring text = L"1st line\n2nd line\n3rd line\n4th line";
  buffer->InsertText(TextPoint(0, 1), text);

  size_t i = 0;
  TextBuffer::CharIterator ci(buffer->CharBegin());
  for (; ci != buffer->CharEnd(); ++ci, ++i) {
    EXPECT_EQ(text[i], *ci);
  }

  EXPECT_EQ(TextPoint(buffer->LineLength(4), 4),
            buffer->PointFromCharIterator(ci));

  --ci;
  EXPECT_EQ(TextPoint(buffer->LineLength(4) - 1, 4),
            buffer->PointFromCharIterator(ci));
}

////////////////////////////////////////////////////////////////////////////////
// static TextBuffer* Create(const std::wstring& text,
//                           FtPlugin* ft_plugin,
//                           const Encoding& file_encoding);

TEST(TextBuffer, Create_WithText_EmptyLines) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer;

  std::wstring text;
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));

  text = L"\r";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(2)->IsEmpty(true));

  text = L"\n";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(2)->IsEmpty(true));

  text = L"\r\n";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(2)->IsEmpty(true));

  text = L"\r\r";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(3, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(2)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(3)->IsEmpty(true));

  text = L"\n\n";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(3, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(2)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(3)->IsEmpty(true));

  text = L"\r\n\r\n";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(3, buffer->LineCount());
  EXPECT_TRUE(buffer->Line(1)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(2)->IsEmpty(true));
  EXPECT_TRUE(buffer->Line(3)->IsEmpty(true));
}

TEST(TextBuffer, Create_WithText) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer;

  std::wstring text = L"abc";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->LineData(1));

  text = L"abc\nde";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->LineData(1));
  EXPECT_EQ(L"de", buffer->LineData(2));

  text = L"abc\rde";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->LineData(1));
  EXPECT_EQ(L"de", buffer->LineData(2));

  text = L"abc\r\nde";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->LineData(1));
  EXPECT_EQ(L"de", buffer->LineData(2));

  text = L"abc\r\nde\r\n";
  buffer.reset(TextBuffer::Create(text, &ft_plugin, kEncoding));
  EXPECT_EQ(3, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->LineData(1));
  EXPECT_EQ(L"de", buffer->LineData(2));
  EXPECT_EQ(L"", buffer->LineData(3));
}

TEST(TextBuffer, PrevNonEmptyLine) {
  FtPlugin ft_plugin(FileType("cpp", "C++"));
  ft_plugin.AddQuote(new Quote(kLexComment, L"//", L"", kQuoteEscapeEol));
  ft_plugin.AddQuote(new Quote(kLexComment, L"/*", L"*/", kQuoteMultiLine));

  TextBufferPtr buffer;
  buffer.reset(TextBuffer::Create(&ft_plugin, kEncoding));

  buffer->AppendLine(L"int i;");  // Line 2
  buffer->AppendLine(L"while(true) {} // nothing");
  buffer->AppendLine(L"// line comments");
  buffer->AppendLine(L"  \t// line comments");
  buffer->AppendLine(L"");
  buffer->AppendLine(L" /* test */ /* test */  ");
  buffer->AppendLine(L"/* block");
  buffer->AppendLine(L"   comments */");
  buffer->AppendLine(L"  \t");
  buffer->AppendLine(L"int j;");  // Line 11

  EXPECT_EQ(3, buffer->PrevNonEmptyLine(11, true));
  EXPECT_EQ(9, buffer->PrevNonEmptyLine(11, false));
}

TEST(TextBuffer, PrevLine) {
  FtPlugin ft_plugin(FileType("cpp", "C++"));
  ft_plugin.AddQuote(new Quote(kLexComment, L"//", L"", kQuoteEscapeEol));
  ft_plugin.AddQuote(new Quote(kLexComment, L"/*", L"*/", kQuoteMultiLine));

  TextBufferPtr buffer;
  buffer.reset(TextBuffer::Create(&ft_plugin, kEncoding));

  buffer->AppendLine(L"    class {");         // Line 2
  buffer->AppendLine(L"");                    // Empty line
  buffer->AppendLine(L"  \t");                // Empty line
  buffer->AppendLine(L"// line comments");
  buffer->AppendLine(L"  \t// line comments");
  buffer->AppendLine(L"    public:");

  EXPECT_EQ(2, buffer->PrevLine(7, LineStartWith(L"class")));
}

////////////////////////////////////////////////////////////////////////////////

// Helper buffer listener to test notify.
class TestBufferListener : public BufferListener {
public:
  TestBufferListener() {
  }

  virtual ~TestBufferListener() {
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
  FtPlugin ft_plugin(kFtTxt);

  TestBufferListener listener;

  TextBufferPtr buffer(TextBuffer::Create(L"line 1", &ft_plugin, kEncoding));
  buffer->AttachListener(&listener);

  buffer->AppendLine(L"line 2");

  EXPECT_EQ(kLineAdded, listener.line_change_type());

  const LineChangeData& line_change_data = listener.line_change_data();
  EXPECT_EQ(2, line_change_data.first());
  EXPECT_EQ(2, line_change_data.last());
}

////////////////////////////////////////////////////////////////////////////////

//TEST(TextBuffer, SearchRegEx_CrossLine) {
//  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(NULL, kEncoding));
//
//  std::wstring text = L"1\n22 \"test\n comments\"\n333";
//  buffer->InsertLine(1, L"1");
//  buffer->InsertLine(2, L"22 \"test\n comments");
//  buffer->InsertLine(3, L"3\"33");
//
//  FindOptions fo = { true, true, false, true };
//  TextRange range = buffer->Find(L"(\".*\")", buffer->range(), fo);
//
//  EXPECT_EQ(TextPoint(3, 2), range.point_begin());
//  EXPECT_EQ(TextPoint(1, 3), range.point_end());
//}
