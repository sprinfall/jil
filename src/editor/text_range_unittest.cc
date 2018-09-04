#include "editor/text_range.h"
#include "gtest/gtest.h"

using namespace editor;

TEST(LineRange, IntersectWith_Empty) {
  LineRange lr1;
  LineRange lr2;
  EXPECT_FALSE(lr1.IntersectWith(lr2));
}

TEST(LineRange, IntersectWith) {
  LineRange lr1(1, 3);
  LineRange lr2(5, 8);
  EXPECT_FALSE(lr1.IntersectWith(lr2));

  lr2.Set(3, 5);
  EXPECT_TRUE(lr1.IntersectWith(lr2));

  lr1.Set(4, 6);
  EXPECT_TRUE(lr1.IntersectWith(lr2));
}

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
