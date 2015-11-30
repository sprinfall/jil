#include <memory>
#include <vector>
#include "editor/action.h"
#include "editor/defs.h"
#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"
#include "editor/util.h"
#include "gtest/gtest.h"

using namespace jil::editor;

typedef std::auto_ptr<TextBuffer> TextBufferPtr;

static const FileType kFtTxt("txt", "Text");
static const Encoding kEncoding = GetEncodingById(ENCODING_ISO_8859_1);

////////////////////////////////////////////////////////////////////////////////
// InsertCharAction

TEST(InsertCharAction, InsertChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, &ft_plugin, kEncoding));

  TextPoint point(0, 1);
  InsertCharAction ica(buffer.get(), point, L'a');

  for (int i = 0; i < 2; ++i) {
    ica.Exec();

    EXPECT_TRUE(ica.effective());

    EXPECT_EQ(TextPoint(1, 0), ica.delta_point());
    EXPECT_EQ(L"a", buffer->LineData(1));

    ica.Undo();

    EXPECT_EQ(L"", buffer->LineData(1));
  }
}

TEST(InsertCharAction, InsertLineEndingChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc", &ft_plugin, kEncoding));

  {
    TextPoint point(3, 1);
    InsertCharAction ica(buffer.get(), point, LF);

    for (int i = 0; i < 2; ++i) {
      ica.Exec();

      EXPECT_TRUE(ica.effective());

      EXPECT_EQ(TextPoint(-3, 1), ica.delta_point());
      EXPECT_EQ(2, buffer->LineCount());
      EXPECT_EQ(L"abc", buffer->LineData(1));
      EXPECT_EQ(L"", buffer->LineData(2));

      ica.Undo();

      EXPECT_EQ(1, buffer->LineCount());
      EXPECT_EQ(L"abc", buffer->LineData(1));
    }
  }

  {
    TextPoint point(1, 1);
    InsertCharAction ica(buffer.get(), point, LF);

    for (int i = 0; i < 2; ++i) {
      ica.Exec();

      EXPECT_TRUE(ica.effective());

      EXPECT_EQ(TextPoint(-1, 1), ica.delta_point());
      EXPECT_EQ(2, buffer->LineCount());
      EXPECT_EQ(L"a", buffer->LineData(1));
      EXPECT_EQ(L"bc", buffer->LineData(2));

      ica.Undo();

      EXPECT_EQ(1, buffer->LineCount());
      EXPECT_EQ(L"abc", buffer->LineData(1));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// InsertStringAction

TEST(InsertStringAction, InsertString) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, &ft_plugin, kEncoding));

  TextPoint point(0, 1);
  InsertStringAction isa(buffer.get(), point, L"abc");

  for (int i = 0; i < 2; ++i) {
    isa.Exec();

    EXPECT_TRUE(isa.effective());

    EXPECT_EQ(TextPoint(3, 0), isa.delta_point());
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));

    isa.Undo();

    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"", buffer->LineData(1));
  }
}

////////////////////////////////////////////////////////////////////////////////
// DeleteAction

//------------------------------------------------------------------------------
// Delete Char

TEST(DeleteAction, DeletePrevChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"a", &ft_plugin, kEncoding));

  TextPoint point(1, 1);
  DeleteAction da(buffer.get(), point, kChar, kPrev);

  for (int i = 0; i < 2; ++i) {
    da.Exec();

    EXPECT_TRUE(da.effective());

    EXPECT_EQ(TextPoint(-1, 0), da.delta_point());
    EXPECT_EQ(L"", buffer->LineData(1));

    da.Undo();

    EXPECT_EQ(L"a", buffer->LineData(1));
  }
}

TEST(DeleteAction, DeletePrevLineEndingChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc\nde", &ft_plugin, kEncoding));

  TextPoint point(0, 2);
  DeleteAction da(buffer.get(), point, kChar, kPrev);

  for (int i = 0; i < 2; ++i) {
    da.Exec();

    EXPECT_TRUE(da.effective());

    EXPECT_EQ(TextPoint(3, -1), da.delta_point());
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abcde", buffer->LineData(1));

    da.Undo();

    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"de", buffer->LineData(2));
  }
}

TEST(DeleteAction, DeletePrevLineEndingChar_Negative) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc", &ft_plugin, kEncoding));

  TextPoint point(0, 1);
  DeleteAction da(buffer.get(), point, kChar, kPrev);

  for (int i = 0; i < 2; ++i) {
    da.Exec();

    // Nothing is deleted.
    EXPECT_FALSE(da.effective());

    EXPECT_EQ(TextPoint(0, 0), da.delta_point());
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));

    da.Undo();

    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
  }
}

TEST(DeleteAction, DeleteNextChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"a", &ft_plugin, kEncoding));

  TextPoint point(0, 1);
  DeleteAction da(buffer.get(), point, kChar, kNext);

  for (int i = 0; i < 2; ++i) {
    da.Exec();

    EXPECT_TRUE(da.effective());

    EXPECT_EQ(TextPoint(0, 0), da.delta_point());
    EXPECT_EQ(L"", buffer->LineData(1));

    da.Undo();

    EXPECT_EQ(L"a", buffer->LineData(1));
  }
}

TEST(DeleteAction, DeleteNextLineEndingChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc\nde", &ft_plugin, kEncoding));

  TextPoint point(3, 1);
  DeleteAction da(buffer.get(), point, kChar, kNext);

  for (int i = 0; i < 2; ++i) {
    da.Exec();

    EXPECT_TRUE(da.effective());

    EXPECT_EQ(TextPoint(0, 0), da.delta_point());
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abcde", buffer->LineData(1));

    da.Undo();

    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"de", buffer->LineData(2));
  }
}

TEST(DeleteAction, DeleteNextLineEndingChar_Negative) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc", &ft_plugin, kEncoding));

  TextPoint point(3, 1);
  DeleteAction da(buffer.get(), point, kChar, kNext);
  
  for (int i = 0; i < 2; ++i) {
    da.Exec();

    EXPECT_FALSE(da.effective());

    EXPECT_EQ(TextPoint(0, 0), da.delta_point());
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));

    da.Undo();

    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
  }
}

////////////////////////////////////////////////////////////////////////////////
// DeleteRangeAction

TEST(DeleteRangeAction, DeleteRange_Forward) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc\ndef\nghi\n", &ft_plugin, kEncoding));

  // a{bc
  // def
  // g}hi
  TextRange range(TextPoint(1, 1), TextPoint(1, 3));

  DeleteRangeAction dra(buffer.get(), range, kForward, false, false);

  for (int i = 0; i < 2; ++i) {
    dra.Exec();

    EXPECT_TRUE(dra.effective());

    TextPoint delta_point = range.point_begin() - range.point_end();
    EXPECT_EQ(delta_point, dra.delta_point());

    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"ahi", buffer->LineData(1));
    EXPECT_EQ(L"", buffer->LineData(2));

    dra.Undo();

    EXPECT_EQ(4, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"def", buffer->LineData(2));
    EXPECT_EQ(L"ghi", buffer->LineData(3));
    EXPECT_EQ(L"", buffer->LineData(4));
  }
}

TEST(DeleteRangeAction, DeleteRange_Backward) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(0, L"abc\ndef\nghi\n", &ft_plugin, kEncoding));

  // a{bc
  // def
  // g}hi
  TextRange range(TextPoint(1, 1), TextPoint(1, 3));

  DeleteRangeAction dra(buffer.get(), range, kBackward, false, false);

  for (int i = 0; i < 2; ++i) {
    dra.Exec();

    EXPECT_TRUE(dra.effective());

    EXPECT_EQ(TextPoint(0, 0), dra.delta_point());

    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"ahi", buffer->LineData(1));
    EXPECT_EQ(L"", buffer->LineData(2));

    dra.Undo();

    EXPECT_EQ(4, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->LineData(1));
    EXPECT_EQ(L"def", buffer->LineData(2));
    EXPECT_EQ(L"ghi", buffer->LineData(3));
    EXPECT_EQ(L"", buffer->LineData(4));
  }
}

////////////////////////////////////////////////////////////////////////////////
// RetabAction

TEST(RetabAction, ToSpaces) {
  FtPlugin ft_plugin(kFtTxt);

  TextOptions text_options;
  text_options.expand_tab = true;
  text_options.tab_stop = 4;
  ft_plugin.set_text_options(text_options);

  TextBufferPtr buffer(TextBuffer::Create(0, &ft_plugin, kEncoding));

  buffer->AppendLine(L"test");
  buffer->AppendLine(L"\ttest");
  buffer->AppendLine(L"  \ttest");
  buffer->AppendLine(L"\t  test");
  buffer->AppendLine(L"  test");

  const TextPoint caret_point(1, 2);

  RetabAction action(buffer.get());
  action.set_caret_point(caret_point);
  action.set_update_caret(true);

  for (size_t i = 0; i < 2; ++i) {
    action.Exec();

    EXPECT_EQ(L"", buffer->Line(2)->GetIndentStr());
    EXPECT_EQ(L"    ", buffer->Line(3)->GetIndentStr());
    EXPECT_EQ(L"    ", buffer->Line(4)->GetIndentStr());
    EXPECT_EQ(L"      ", buffer->Line(5)->GetIndentStr());
    EXPECT_EQ(L"  ", buffer->Line(6)->GetIndentStr());

    EXPECT_EQ(caret_point, action.CaretPointAfterExec());

    action.Undo();

    EXPECT_EQ(L"", buffer->Line(2)->GetIndentStr());
    EXPECT_EQ(L"\t", buffer->Line(3)->GetIndentStr());
    EXPECT_EQ(L"  \t", buffer->Line(4)->GetIndentStr());
    EXPECT_EQ(L"\t  ", buffer->Line(5)->GetIndentStr());
    EXPECT_EQ(L"  ", buffer->Line(6)->GetIndentStr());
  }
}

TEST(RetabAction, ToTabs) {
  FtPlugin ft_plugin(kFtTxt);

  TextOptions text_options;
  text_options.expand_tab = false;
  text_options.tab_stop = 4;
  ft_plugin.set_text_options(text_options);

  TextBufferPtr buffer(TextBuffer::Create(0, &ft_plugin, kEncoding));

  buffer->AppendLine(L"test");
  buffer->AppendLine(L"    test");
  buffer->AppendLine(L"  \ttest");
  buffer->AppendLine(L"\t  test");
  buffer->AppendLine(L"  test");
  buffer->AppendLine(L"      test");

  const TextPoint caret_point(10, 7);

  RetabAction action(buffer.get());
  action.set_caret_point(caret_point);
  action.set_update_caret(true);

  for (size_t i = 0; i < 2; ++i) {
    action.Exec();

    EXPECT_EQ(L"", buffer->Line(2)->GetIndentStr());
    EXPECT_EQ(L"\t", buffer->Line(3)->GetIndentStr());
    EXPECT_EQ(L"\t", buffer->Line(4)->GetIndentStr());
    EXPECT_EQ(L"\t  ", buffer->Line(5)->GetIndentStr());
    EXPECT_EQ(L"  ", buffer->Line(6)->GetIndentStr());
    EXPECT_EQ(L"\t  ", buffer->Line(7)->GetIndentStr());

    EXPECT_EQ(TextPoint(7, 7), action.CaretPointAfterExec());

    action.Undo();

    EXPECT_EQ(L"", buffer->Line(2)->GetIndentStr());
    EXPECT_EQ(L"    ", buffer->Line(3)->GetIndentStr());
    EXPECT_EQ(L"  \t", buffer->Line(4)->GetIndentStr());
    EXPECT_EQ(L"\t  ", buffer->Line(5)->GetIndentStr());
    EXPECT_EQ(L"  ", buffer->Line(6)->GetIndentStr());
    EXPECT_EQ(L"      ", buffer->Line(7)->GetIndentStr());
  }
}