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
static const Encoding kEncoding = EncodingFromName(ENCODING_NAME_ISO_8859_1);

////////////////////////////////////////////////////////////////////////////////
// InsertCharAction

TEST(InsertCharAction, InsertChar) {
  FtPlugin ft_plugin(kFtTxt);
  TextBufferPtr buffer(TextBuffer::Create(&ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(&ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"a", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc\nde", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"a", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc\nde", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc\ndef\nghi\n", &ft_plugin, kEncoding));

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
  TextBufferPtr buffer(TextBuffer::Create(L"abc\ndef\nghi\n", &ft_plugin, kEncoding));

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
