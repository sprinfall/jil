#include <memory>
#include "editor/text_buffer.h"
#include "editor/wrap.h"
#include "editor/text_extent.h"
#include "base/compiler_specific.h"
#include "gtest/gtest.h"

using namespace jil::editor;

static const int kCharWidth = 9;
/*

TEST(WrapHelper, WrapLineCount1) {
  TestTextExtent text_extent;
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create());
  WrapHelper wrap_helper(buffer.get());

  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC
  //  2 DEF
  //  3 GHI
  //  4 J
  //  5 KLM
  //  6 N
  buffer->SetText(_T("ABC\nDEFGHIJ\nKLMN"));
  wrap_helper.Wrap(text_extent, 3 * kCharWidth);

  EXPECT_EQ(6, wrap_helper.WrapLineCount());
}

TEST(TextBuffer, WrapLineCount2) {
  TestTextExtent text_extent;
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create());
  WrapHelper wrap_helper(buffer.get());

  buffer->SetText(_T("ABC\nDEF\nGH"));
  wrap_helper.Wrap(text_extent, 3 * kCharWidth);

  EXPECT_EQ(3, wrap_helper.WrapLineCount());
}

TEST(TextBuffer, WrapTextRange) {
  TestTextExtent text_extent;
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create());
  WrapHelper wrap_helper(buffer.get());

  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC  - 1
  //  2 DEF  \
  //  3 GHI   2
  //  4 J    /
  //  5 KLM  \ 3
  //  6 N    /
  buffer->SetText(_T("ABC\nDEFGHIJ\nKLMN"));
  wrap_helper.Wrap(text_extent, 3 * kCharWidth);

  {
    TextRange wrap_line_range = wrap_helper.WrapTextRange(TextRange(1, 1));
    EXPECT_EQ(1, wrap_line_range.line_begin());
    EXPECT_EQ(1, wrap_line_range.line_end());
  }

  {
    TextRange wrap_line_range = wrap_helper.WrapTextRange(TextRange(1, 2));
    EXPECT_EQ(1, wrap_line_range.line_begin());
    EXPECT_EQ(4, wrap_line_range.line_end());
  }

  {
    TextRange wrap_line_range = wrap_helper.WrapTextRange(TextRange(2, 3));
    EXPECT_EQ(2, wrap_line_range.line_begin());
    EXPECT_EQ(6, wrap_line_range.line_end());
  }

  {
    TextRange wrap_line_range = wrap_helper.WrapTextRange(TextRange(3, 3));
    EXPECT_EQ(5, wrap_line_range.line_begin());
    EXPECT_EQ(6, wrap_line_range.line_end());
  }

  {
    // Line end(4) doesn't exist.
    TextRange wrap_line_range = wrap_helper.WrapTextRange(TextRange(3, 4));
    EXPECT_EQ(5, wrap_line_range.line_begin());
    EXPECT_EQ(6, wrap_line_range.line_end());
  }

  {
    // Line begin(4) and end(5) doesn't exist.
    TextRange wrap_line_range = wrap_helper.WrapTextRange(TextRange(4, 5));
    EXPECT_EQ(0, wrap_line_range.line_begin());
    EXPECT_EQ(0, wrap_line_range.line_end());
  }
}

TEST(TextBuffer, UnWrapTextRange) {
  TestTextExtent text_extent;
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create());
  WrapHelper wrap_helper(buffer.get());

  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC  - 1
  //  2 DEF  \
  //  3 GHI   2
  //  4 J    /
  //  5 KLM  \ 3
  //  6 N    /
  buffer->SetText(_T("ABC\nDEFGHIJ\nKLMN"));
  wrap_helper.Wrap(text_extent, 3 * kCharWidth);

  {
    TextRange text_range = wrap_helper.UnWrapTextRange(TextRange(1, 1));

    EXPECT_EQ(1, text_range.line_begin());
    EXPECT_EQ(1, text_range.line_end());
  }

  {
    TextRange text_range = wrap_helper.UnWrapTextRange(TextRange(1, 2));

    EXPECT_EQ(1, text_range.line_begin());
    EXPECT_EQ(2, text_range.line_end());
  }

  {
    TextRange text_range = wrap_helper.UnWrapTextRange(TextRange(1, 3));

    EXPECT_EQ(1, text_range.line_begin());
    EXPECT_EQ(2, text_range.line_end());
  }

  {
    TextRange text_range = wrap_helper.UnWrapTextRange(TextRange(2, 4));

    EXPECT_EQ(2, text_range.line_begin());
    EXPECT_EQ(2, text_range.line_end());
  }

  {
    // Wrap line 8 doesn't belong to a unwrap line.
    TextRange text_range = wrap_helper.UnWrapTextRange(TextRange(2, 8));

    EXPECT_EQ(2, text_range.line_begin());
    EXPECT_EQ(3, text_range.line_end());
  }

  {
    TextRange text_range = wrap_helper.UnWrapTextRange(TextRange(10, 20));

    EXPECT_EQ(0, text_range.line_begin());
    EXPECT_EQ(0, text_range.line_end());
  }
}
*/