#include "editor/relite.h"
#include "gtest/gtest.h"

using namespace jil::editor;

TEST(LiteRegex, Match_Basic) {
  std::wstring pattern = L"#\\s*elif\\s+!?\\s*defined";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"#elif defined";
  size_t off = regex.Match(str, 0);
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

TEST(LiteRegex, Match_Wildcard) {
  std::wstring pattern = L"#\\s*include\\s*<.+>";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"#include <string>";
  size_t off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);
}

TEST(LiteRegex, Match_Boundary) {
  std::wstring pattern = L"#\\s*elif\\s+!?\\s*defined\\b";

  relite::Regex regex(pattern, 0);

  EXPECT_TRUE(regex.valid());

  std::wstring str = L"#elif defined";
  size_t off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#elif defined ";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#elif defined\t";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#elif defined/";
  off = regex.Match(str, 0);
  EXPECT_EQ(str.size(), off);

  str = L"#elif definedx";
  off = regex.Match(str, 0);
  EXPECT_EQ(0, off);
}
