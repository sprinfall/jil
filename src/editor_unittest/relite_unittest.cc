#include "editor/relite.h"
#include "gtest/gtest.h"

using namespace editor;

TEST(LiteRegex, Match_Basic) {
  std::wstring pattern = L"#\\s*elif\\s+!?\\s*defined";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"#elif defined";
  std::size_t off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"# \telif   defined";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#elif !defined";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#\t elif  !\tdefined";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);
}

TEST(LiteRegex, Match_RepeatNormalChar) {
  std::wstring pattern = L"https?://";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"http://";
  std::size_t off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"https://";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);
}

TEST(LiteRegex, Match_Wildcard) {
  std::wstring pattern = L"#\\s*include\\s*<.+>";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"#include <string>";
  std::size_t off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);
}

TEST(LiteRegex, Match_Boundary) {
  std::wstring pattern = L"#\\s*elif\\s+!?\\s*defined\\b";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"#elif defined";
  std::size_t off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#elif defined ";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size() - 1, off);

  str = L"#elif defined\t";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size() - 1, off);

  str = L"#elif defined/";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size() - 1, off);

  str = L"#elif definedx";
  off = regex.Match(str, 0);
  EXPECT_EQ(0, off);
}

TEST(LiteRegex, Match_SubExpression) {
  std::wstring pattern = L"--\\[(=+)\\[";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  relite::Sub sub;

  std::wstring str = L"--[===[";
  std::size_t off = regex.Match(str, 0, &sub, 1);
  EXPECT_EQ(str.size(), off);
  EXPECT_EQ(3, sub.off);
  EXPECT_EQ(3, sub.len);

  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);
}

TEST(LiteRegex, Match_MultiSubExpression) {
  std::wstring pattern = L"--\\[(=+)\\[(\\*+)";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"--[===[**";

  {
    relite::Sub subs[2];
    std::size_t off = regex.Match(str, 0, subs, 2);
    EXPECT_EQ(str.size(), off);
    EXPECT_EQ(3, subs[0].off);
    EXPECT_EQ(3, subs[0].len);
    EXPECT_EQ(7, subs[1].off);
    EXPECT_EQ(2, subs[1].len);
  }

  {
    relite::Sub sub;
    std::size_t off = regex.Match(str, 0, &sub, 1);
    EXPECT_EQ(str.size(), off);
    EXPECT_EQ(3, sub.off);
    EXPECT_EQ(3, sub.len);
  }

  {
    std::size_t off = regex.Match(str, 0);
    EXPECT_EQ(str.size(), off);
  }
}

TEST(LiteRegex, Match_SubExpression_Negative) {
  {
    relite::Regex regex(L"--\\[(=+\\[", 0);
    EXPECT_FALSE(regex.valid());
  }

  {
    relite::Regex regex(L"--\\[=+)\\[", 0);
    EXPECT_FALSE(regex.valid());
  }
}


TEST(LiteRegex, Match_IgnoreCase) {
  relite::Regex regex(L"a+", relite::kIgnoreCase);
  EXPECT_TRUE(regex.valid());

  {
    std::wstring str = L"aaa";
    std::size_t off = regex.Match(str, 0);
    EXPECT_EQ(str.size(), off);
  }

  {
    std::wstring str = L"AAA";
    std::size_t off = regex.Match(str, 0);
    EXPECT_EQ(str.size(), off);
  }
}


TEST(LiteRegex, Match_End) {
  {
    relite::Regex regex(L"\\s+$x", 0);
    EXPECT_FALSE(regex.valid());
  }

  {
    relite::Regex regex(L"test$", 0);
    EXPECT_TRUE(regex.valid());

    std::wstring str = L"test";
    std::size_t off = regex.Match(str, 0);
    EXPECT_EQ(str.size(), off);

    str = L"  test";
    off = regex.Match(str, 2);
    EXPECT_EQ(str.size(), off);

    str = L"  test ";
    off = regex.Match(str, 2);
    EXPECT_EQ(2, off);
  }

  {
    relite::Regex regex(L"\\s+$", 0);
    EXPECT_TRUE(regex.valid());

    std::wstring str = L"test   ";
    std::size_t off = regex.Match(str, 4);
    EXPECT_EQ(str.size(), off);

    str = L"  x";
    off = regex.Match(str, 0);
    EXPECT_EQ(0, off);
  }
}