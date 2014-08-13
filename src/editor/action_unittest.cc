#include <memory>
#include <vector>
#include "editor/edit_action.h"
#include "editor/text_buffer.h"
#include "gtest/gtest.h"

using namespace jil::editor;

////////////////////////////////////////////////////////////////////////////////
// InsertCharAction

TEST(EditAction, InsertChar) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(wxEmptyString, NULL, wxEmptyString));

  TextPoint insert_coord(0, 1);
  InsertCharAction insert_char(buffer.get(), insert_coord, L'a');
  insert_char.Exec();

  EXPECT_EQ(TextPoint(1, 0), insert_char.delta_point());
  EXPECT_EQ(L"a", buffer->GetLine(1).data());

  insert_char.Undo();
}

TEST(EditAction, InsertChar_LineEnding) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc", NULL, wxEmptyString));

  {
    const TextPoint coord(3, 1);
    InsertCharAction insert_char(buffer.get(), coord, kLineEndingChar);
    insert_char.Exec();

    EXPECT_EQ(TextPoint(-3, 1), insert_char.delta_point());
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->GetLine(1).data());
    EXPECT_EQ(L"", buffer->GetLine(2).data());

    insert_char.Undo();
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  }

  {
    const TextPoint coord(1, 1);
    InsertCharAction insert_char(buffer.get(), coord, kLineEndingChar);
    insert_char.Exec();

    EXPECT_EQ(TextPoint(-1, 1), insert_char.delta_point());
    EXPECT_EQ(2, buffer->LineCount());
    EXPECT_EQ(L"a", buffer->GetLine(1).data());
    EXPECT_EQ(L"bc", buffer->GetLine(2).data());

    insert_char.Undo();
    EXPECT_EQ(1, buffer->LineCount());
    EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  }
}

////////////////////////////////////////////////////////////////////////////////
// DeleteChar

TEST(EditAction, DeleteChar_Backward) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"a", NULL, wxEmptyString));

  const TextPoint coord(1, 1);
  DeleteCharAction delete_char(buffer.get(), coord, kBackward);
  delete_char.Exec();

  EXPECT_EQ(TextPoint(-1, 0), delete_char.delta_point());
  EXPECT_EQ(L"", buffer->GetLine(1).data());

  delete_char.Undo();
  EXPECT_EQ(L"a", buffer->GetLine(1).data());
}

TEST(EditAction, DeleteChar_Backward_LineEnding) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc\nde", NULL, wxEmptyString));

  const TextPoint coord(0, 2);
  DeleteCharAction delete_char(buffer.get(), coord, kBackward);
  delete_char.Exec();

  EXPECT_EQ(TextPoint(3, -1), delete_char.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abcde", buffer->GetLine(1).data());

  delete_char.Undo();
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  EXPECT_EQ(L"de", buffer->GetLine(2).data());
}

TEST(EditAction, DeleteChar_Backward_LineEnding_Negative) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc", NULL, wxEmptyString));

  const TextPoint coord(0, 1);
  DeleteCharAction delete_char(buffer.get(), coord, kBackward);
  delete_char.Exec();

  // Nothing is deleted.
  EXPECT_EQ(TextPoint(0, 0), delete_char.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());

  delete_char.Undo();
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
}

TEST(EditAction, DeleteChar_Forward) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"a", NULL, wxEmptyString));

  const TextPoint coord(0, 1);
  DeleteCharAction delete_char(buffer.get(), coord, kForward);
  delete_char.Exec();

  EXPECT_EQ(TextPoint(0, 0), delete_char.delta_point());
  EXPECT_EQ(L"", buffer->GetLine(1).data());

  delete_char.Undo();
  EXPECT_EQ(L"a", buffer->GetLine(1).data());
}

TEST(EditAction, DeleteChar_Forward_LineEnding) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc\nde", NULL, wxEmptyString));

  const TextPoint coord(3, 1);
  DeleteCharAction delete_char(buffer.get(), coord, kForward);
  delete_char.Exec();

  EXPECT_EQ(TextPoint(0, 0), delete_char.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abcde", buffer->GetLine(1).data());

  delete_char.Undo();
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  EXPECT_EQ(L"de", buffer->GetLine(2).data());
}

TEST(EditAction, DeleteChar_Forward_LineEnding_Negative) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc", NULL, wxEmptyString));

  const TextPoint coord(3, 1);
  DeleteCharAction delete_char(buffer.get(), coord, kForward);
  delete_char.Exec();

  EXPECT_EQ(TextPoint(0, 0), delete_char.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());

  delete_char.Undo();
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
}

////////////////////////////////////////////////////////////////////////////////
// InsertString

TEST(EditAction, InsertString) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(wxEmptyString, NULL, wxEmptyString));

  const TextPoint coord(0, 1);
  InsertStringAction insert_string(buffer.get(), L"abc", coord);
  insert_string.Exec();

  EXPECT_EQ(TextPoint(3, 0), insert_string.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());

  insert_string.Undo();
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"", buffer->GetLine(1).data());
}

////////////////////////////////////////////////////////////////////////////////
// DeleteLine

TEST(EditAction, DeleteLine) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc\nde", NULL, wxEmptyString));

  const TextPoint coord(3, 1);
  DeleteLineAction delete_line(buffer.get(), coord);
  delete_line.Exec();

  EXPECT_EQ(TextPoint(-1, 0), delete_line.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"de", buffer->GetLine(1).data());

  delete_line.Undo();
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  EXPECT_EQ(L"de", buffer->GetLine(2).data());
}

TEST(EditAction, DeleteLine_LastLine) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc\nde", NULL, wxEmptyString));

  const TextPoint coord(2, 2);
  DeleteLineAction delete_line(buffer.get(), coord);
  delete_line.Exec();

  EXPECT_EQ(TextPoint(-2, 0), delete_line.delta_point());
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  EXPECT_EQ(L"", buffer->GetLine(2).data());

  //delete_line.Undo();
  //EXPECT_EQ(2, buffer->LineCount());
  //EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  //EXPECT_EQ(L"de", buffer->GetLine(2).data());
}

TEST(EditAction, DeleteLine_LastLine2) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc", NULL, wxEmptyString));

  const TextPoint coord(1, 1);
  DeleteLineAction delete_line(buffer.get(), coord);
  delete_line.Exec();

  EXPECT_EQ(TextPoint(-1, 0), delete_line.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"", buffer->GetLine(1).data());

  delete_line.Undo();
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
}

TEST(EditAction, DeleteLine_LastLine3) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(wxEmptyString, NULL, wxEmptyString));

  const TextPoint coord(0, 1);
  DeleteLineAction delete_line(buffer.get(), coord);
  delete_line.Exec();

  EXPECT_EQ(TextPoint(0, 0), delete_line.delta_point());
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"", buffer->GetLine(1).data());

  delete_line.Undo();
  EXPECT_EQ(1, buffer->LineCount());
  EXPECT_EQ(L"", buffer->GetLine(1).data());
}

////////////////////////////////////////////////////////////////////////////////
// DeleteSegment

TEST(EditAction, DeleteSegment) {
  std::auto_ptr<TextBuffer> buffer(TextBuffer::Create(L"abc\ndef\nghi\n", NULL, wxEmptyString));

  // a{bc
  // def
  // g}hi
  //
  const TextPoint coord_begin(1, 1);
  const TextPoint coord_end(1, 3);
  DeleteAction delete_action(buffer.get(), coord_begin, coord_end);
  delete_action.Exec();

  EXPECT_EQ(TextPoint(0, 0), delete_action.delta_point());
  EXPECT_EQ(2, buffer->LineCount());
  EXPECT_EQ(L"ahi", buffer->GetLine(1).data());
  EXPECT_EQ(L"", buffer->GetLine(2).data());

  delete_action.Undo();
  EXPECT_EQ(4, buffer->LineCount());
  EXPECT_EQ(L"abc", buffer->GetLine(1).data());
  EXPECT_EQ(L"def", buffer->GetLine(2).data());
  EXPECT_EQ(L"ghi", buffer->GetLine(3).data());
  EXPECT_EQ(L"", buffer->GetLine(4).data());
}
