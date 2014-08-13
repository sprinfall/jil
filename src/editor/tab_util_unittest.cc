#include "editor/tab.h"
#include "gtest/gtest.h"

using namespace jil::editor;

TEST(TextLine, TabbedLineFast) {
  std::wstring line;
  EXPECT_EQ(std::wstring(L""), TabbedLineFast(4, line));

  line = L"\t";
  EXPECT_EQ(std::wstring(L"    "), TabbedLineFast(4, line));

  line = L"\t\t";
  EXPECT_EQ(std::wstring(L"        "), TabbedLineFast(4, line));

  line = L"a\t";
  EXPECT_EQ(std::wstring(L"a   "), TabbedLineFast(4, line));

  line = L"ab\tc\tde";
  EXPECT_EQ(std::wstring(L"ab c de   "), TabbedLineFast(4, line));
}
