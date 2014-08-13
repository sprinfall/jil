#include <memory>
#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/util.h"
#include "gtest/gtest.h"

using namespace jil::editor;

typedef std::auto_ptr<TextBuffer> TextBufferPtr;

static const Encoding kEncoding = EncodingFromName(ENCODING_NAME_ISO_8859_1);

//TEST(TextBuffer, CharIterator) {
//  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(NULL, kEncoding));
//
//  std::wstring text = L"1st line\n2nd line\n3rd line\n4th line";
//  buffer->InsertText(TextPoint(0, 1), text);
//
//  EXPECT_EQ(4, buffer->LineCount());
//  EXPECT_EQ(std::wstring(L"1st line"), buffer->GetLine(1).data());
//
//  size_t i = 0;
//  TextBuffer::CharIterator it(buffer->begin());
//  for (; it != buffer->end(); ++it, ++i) {
//    EXPECT_EQ(text[i], *it);
//  }
//
//  EXPECT_EQ(TextPoint(buffer->GetLine(4).Length(), 4), buffer->PointFromCharIterator(it));
//
//  --it;
//  EXPECT_EQ(TextPoint(buffer->GetLine(4).Length() - 1, 4), buffer->PointFromCharIterator(it));
//}

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

////////////////////////////////////////////////////////////////////////////////
// static TextBuffer* Create(const std::wstring& text,
//                           FtPlugin* ft_plugin,
//                           const Encoding& file_encoding);

TEST(TextBuffer, Create_WithText_EmptyLines) {
  FileType ft("txt", "Text");
  FtPlugin ft_plugin(ft);

  {
    TextBufferPtr buffer(TextBuffer::Create(L"", &ft_plugin, kEncoding));
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"\r", &ft_plugin, kEncoding));
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
    EXPECT_TRUE(buffer->Line(2)->IsEmpty());
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"\n", &ft_plugin, kEncoding));
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
    EXPECT_TRUE(buffer->Line(2)->IsEmpty());
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"\r\n", &ft_plugin, kEncoding));
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
    EXPECT_TRUE(buffer->Line(2)->IsEmpty());
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"\r\r", &ft_plugin, kEncoding));
    EXPECT_EQ(3, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
    EXPECT_TRUE(buffer->Line(2)->IsEmpty());
    EXPECT_TRUE(buffer->Line(3)->IsEmpty());
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"\n\n", &ft_plugin, kEncoding));
    EXPECT_EQ(3, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
    EXPECT_TRUE(buffer->Line(2)->IsEmpty());
    EXPECT_TRUE(buffer->Line(3)->IsEmpty());
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"\r\n\r\n", &ft_plugin, kEncoding));
    EXPECT_EQ(3, buffer->LineCount());
    EXPECT_TRUE(buffer->Line(1)->IsEmpty());
    EXPECT_TRUE(buffer->Line(2)->IsEmpty());
    EXPECT_TRUE(buffer->Line(3)->IsEmpty());
  }
}

TEST(TextBuffer, Create_WithText) {
  FileType ft("txt", "Text");
  FtPlugin ft_plugin(ft);

  {
    TextBufferPtr buffer(TextBuffer::Create(L"abc", &ft_plugin, kEncoding));
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"abc\nde", &ft_plugin, kEncoding));
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"de", buffer->LineData(2));
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"abc\rde", &ft_plugin, kEncoding));
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"de", buffer->LineData(2));
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"abc\r\nde", &ft_plugin, kEncoding));
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"de", buffer->LineData(2));
  }

  {
    TextBufferPtr buffer(TextBuffer::Create(L"abc\r\nde\r\n", &ft_plugin, kEncoding));
    EXPECT_EQ(3, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"de", buffer->LineData(2));
    EXPECT_EQ(L"", buffer->LineData(3));
  }
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
  FileType ft("txt", "Text");
  FtPlugin ft_plugin(ft);

  TestBufferListener listener;

  TextBufferPtr buffer(TextBuffer::Create(L"line 1", &ft_plugin, kEncoding));
  buffer->AttachListener(&listener);

  buffer->AppendLine(L"line 2");

  EXPECT_EQ(kLineAdded, listener.line_change_type());

  const LineChangeData& line_change_data = listener.line_change_data();
  EXPECT_EQ(2, line_change_data.first());
  EXPECT_EQ(2, line_change_data.last());
}
