#include "editor/text_range.h"
#include "gtest/gtest.h"

using namespace jil::editor;

TEST(TextRange, IsEmpty) {
  TextRange range;
  EXPECT_TRUE(range.IsEmpty());
}

TEST(TextRange, Set) {
  TextRange range;

  TextPoint point_begin(1, 1);
  TextPoint point_end(0, 3);

  range.Set(point_begin, point_end);

  EXPECT_EQ(point_begin, range.point_begin());
  EXPECT_EQ(point_end, range.point_end());
}
