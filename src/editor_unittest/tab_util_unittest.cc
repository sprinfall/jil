#include "gtest/gtest.h"

#include "editor/tab.h"
//
//TEST(TextLine, TabbedLineFast) {
//  using editor::TabbedLineFast;
//
//  std::wstring line;
//  EXPECT_EQ(std::wstring(L""), TabbedLineFast(4, line));
//
//  line = L"\t";
//  EXPECT_EQ(std::wstring(L"    "), TabbedLineFast(4, line));
//
//  line = L"\t\t";
//  EXPECT_EQ(std::wstring(L"        "), TabbedLineFast(4, line));
//
//  line = L"a\t";
//  EXPECT_EQ(std::wstring(L"a   "), TabbedLineFast(4, line));
//
//  line = L"ab\tc\tde";
//  EXPECT_EQ(std::wstring(L"ab c de   "), TabbedLineFast(4, line));
//}
