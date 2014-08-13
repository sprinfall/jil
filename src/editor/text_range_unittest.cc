#include "editor/text_range.h"
#include "gtest/gtest.h"

using namespace jil::editor;

TEST(TextRange, Valid) {
  TextRange range;
  EXPECT_TRUE(range.Empty());
}

TEST(TextRange, DiffTextRangeByLine1) {
  // 1  * * * * * *
  // 2  *{* *[* * *]* *}
  // 3  * * * * *
  // 4  * * * * * * * * * *
  //TextRange old_range(2, 2, 1, kXCharEnd);
  //TextRange new_range(2, 2, 3, 5);
  TextRange old_range(TextPoint(1, 2), TextPoint(kXCharEnd, 2));
  TextRange new_range(TextPoint(3, 2), TextPoint(5, 2));

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(1, diff_lines.size());
  EXPECT_EQ(2, diff_lines[0]);
}
/*
TEST(TextRange, DiffTextRangeByLine2) {
  // 1  * * * * * *
  // 2 [*{* * * * *]*}*
  // 3  * * * * *
  // 4  * * * * * * * * * *
  TextRange old_range(2, 2, 1, 6);
  TextRange new_range(2, 2, 0, 5);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(1, diff_lines.size());
  EXPECT_EQ(2, diff_lines[0]);
}

TEST(TextRange, DiffTextRangeByLine3) {
  // 1  * * * * * *
  // 2  *[*]* * *{* *}*
  // 3  * * * * *
  // 4  * * * * * * * * * *
  TextRange old_range(2, 2, 5, 6);
  TextRange new_range(2, 2, 1, 2);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(1, diff_lines.size());
  EXPECT_EQ(2, diff_lines[0]);
}

TEST(TextRange, DiffTextRangeByLine4) {
  // 1  * *{*[* * *
  // 2  * * * * * * * *
  // 3  * * * * *
  // 4  * * *]*}* * * * * *
  TextRange old_range(1, 4, 2, 4);
  TextRange new_range(1, 4, 3, 3);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(2, diff_lines.size());
  EXPECT_EQ(1, diff_lines[0]);
  EXPECT_EQ(4, diff_lines[1]);
}

TEST(TextRange, DiffTextRangeByLine5) {
  // 1  * *{* * * *
  // 2  *[* * * * * * *
  // 3  * * * * *
  // 4  * * *]*}* * * * * *
  TextRange old_range(1, 4, 2, 4);
  TextRange new_range(2, 4, 1, 3);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(3, diff_lines.size());
  EXPECT_EQ(1, diff_lines[0]);
  EXPECT_EQ(2, diff_lines[1]);
  EXPECT_EQ(4, diff_lines[2]);
}

TEST(TextRange, DiffTextRangeByLine6) {
  // 1  * *{* * * *
  // 2  *[* * * * * * *
  // 3  * * * *]*
  // 4  * * * *}* * * * * *
  TextRange old_range(1, 4, 2, 4);
  TextRange new_range(2, 3, 1, 3);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(4, diff_lines.size());
  EXPECT_EQ(1, diff_lines[0]);
  EXPECT_EQ(2, diff_lines[1]);
  EXPECT_EQ(3, diff_lines[2]);
  EXPECT_EQ(4, diff_lines[3]);
}

TEST(TextRange, DiffTextRangeByLine7) {
  // 1  * *{* * * *
  // 2  *[* * * * * * *
  // 3  * * * * *]  -- This line doesn't have to refresh!
  // 4  * * * *}* * * * * *
  TextRange old_range(1, 4, 2, 4);
  TextRange new_range(2, 3, 1, kXCharEnd);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(3, diff_lines.size());
  EXPECT_EQ(1, diff_lines[0]);
  EXPECT_EQ(2, diff_lines[1]);
  EXPECT_EQ(4, diff_lines[2]);
}

TEST(TextRange, DiffTextRangeByLine8) {
  // 1  * *{* * * *
  // 2  * * * * * *}* *
  // 3  *[* * * *
  // 4  * * * *]* * * * * *
  TextRange old_range(1, 2, 2, 5);
  TextRange new_range(3, 4, 1, 3);

  std::vector<int> diff_lines;
  DiffTextRangeByLine(old_range, new_range, diff_lines);

  EXPECT_EQ(4, diff_lines.size());
  EXPECT_EQ(1, diff_lines[0]);
  EXPECT_EQ(2, diff_lines[1]);
  EXPECT_EQ(3, diff_lines[2]);
  EXPECT_EQ(4, diff_lines[3]);
}*/