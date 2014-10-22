#include "editor/text_line.h"
#include "gtest/gtest.h"

using namespace jil::editor;

TEST(TextLine, Char) {
  TextLine line(0, L"a b\tc");

  EXPECT_EQ(L'a', line.Char(0));
  EXPECT_EQ(L' ', line.Char(1));
  EXPECT_EQ(L'\n', line.Char(line.Length()));
}

TEST(TextLine, Sub) {
  TextLine line(0, L"a b\tc");

  EXPECT_EQ(L"", line.Sub(0, 0));
  EXPECT_EQ(L"", line.Sub(CharRange(0, 0)));

  EXPECT_EQ(L"a", line.Sub(0, 1));
  EXPECT_EQ(L"a", line.Sub(CharRange(0, 1)));

  EXPECT_EQ(L"b\tc", line.Sub(2, 3));
  EXPECT_EQ(L"b\tc", line.Sub(CharRange(2, 5)));

  EXPECT_EQ(L"b\tc", line.Sub(2, kInvalidCoord));
  EXPECT_EQ(L"b\tc", line.Sub(CharRange(2, kInvalidCoord)));

  EXPECT_EQ(L"a b\tc", line.Sub(0, kInvalidCoord));
  EXPECT_EQ(L"a b\tc", line.Sub(CharRange(0, kInvalidCoord)));
}

TEST(TextLine, FirstNonSpaceChar) {
  TextLine line(0, L"");
  EXPECT_EQ(line.Length(), line.FirstNonSpaceChar(0));

  line.InsertString(0, L"a b\tc");
  EXPECT_EQ(0, line.FirstNonSpaceChar(0));
  EXPECT_EQ(2, line.FirstNonSpaceChar(1));
  EXPECT_EQ(4, line.FirstNonSpaceChar(3));
  EXPECT_EQ(line.Length(), line.FirstNonSpaceChar(line.Length()));
  EXPECT_EQ(line.Length(), line.FirstNonSpaceChar(line.Length() + 1));
}

TEST(TextLine, LastNonSpaceChar) {
  TextLine line(0, L"");
  EXPECT_EQ(kInvalidCoord, line.LastNonSpaceChar());

  line.InsertString(0, L"a b\tc");
  EXPECT_EQ(4, line.LastNonSpaceChar());
  EXPECT_EQ(4, line.LastNonSpaceChar(line.Length()));
  EXPECT_EQ(2, line.LastNonSpaceChar(4));
  EXPECT_EQ(0, line.LastNonSpaceChar(2));
  EXPECT_EQ(0, line.LastNonSpaceChar(1));
  EXPECT_EQ(kInvalidCoord, line.LastNonSpaceChar(0));
}

TEST(TextLine, RangeLexElements) {
  TextLine line(0, L"int i = 0;");
  line.AddLexElement(0, 3, Lex(kLexType));
  line.AddLexElement(8, 1, Lex(kLexConstant, kLexConstantNumber));

  typedef std::list<const LexElement*> LexElements;

  LexElements lex_elements0 = line.lex_elements(CharRange(0, kInvalidCoord));
  EXPECT_EQ(2, lex_elements0.size());

  LexElements lex_elements1 = line.lex_elements(CharRange(1, kInvalidCoord));
  EXPECT_EQ(2, lex_elements1.size());

  LexElements lex_elements2 = line.lex_elements(CharRange(2, kInvalidCoord));
  EXPECT_EQ(2, lex_elements2.size());

  LexElements lex_elements3 = line.lex_elements(CharRange(3, kInvalidCoord));
  EXPECT_EQ(1, lex_elements3.size());

  LexElements lex_elements4 = line.lex_elements(CharRange(4, kInvalidCoord));
  EXPECT_EQ(1, lex_elements4.size());

  LexElements lex_elements5 = line.lex_elements(CharRange(5, kInvalidCoord));
  EXPECT_EQ(1, lex_elements5.size());

  LexElements lex_elements6 = line.lex_elements(CharRange(6, kInvalidCoord));
  EXPECT_EQ(1, lex_elements6.size());

  LexElements lex_elements8 = line.lex_elements(CharRange(8, kInvalidCoord));
  EXPECT_EQ(1, lex_elements8.size());

  LexElements lex_elements9 = line.lex_elements(CharRange(9, kInvalidCoord));
  EXPECT_EQ(0, lex_elements9.size());


  LexElements lex_elements03 = line.lex_elements(CharRange(0, 3));
  EXPECT_EQ(1, lex_elements03.size());

  LexElements lex_elements08 = line.lex_elements(CharRange(0, 8));
  EXPECT_EQ(1, lex_elements08.size());

  LexElements lex_elements09 = line.lex_elements(CharRange(0, 9));
  EXPECT_EQ(2, lex_elements09.size());


  LexElements lex_elements12 = line.lex_elements(CharRange(1, 2));
  EXPECT_EQ(1, lex_elements12.size());

  LexElements lex_elements15 = line.lex_elements(CharRange(1, 5));
  EXPECT_EQ(1, lex_elements15.size());

  LexElements lex_elements89 = line.lex_elements(CharRange(8, 9));
  EXPECT_EQ(1, lex_elements89.size());

  LexElements lex_elements9a = line.lex_elements(CharRange(9, 10));
  EXPECT_EQ(0, lex_elements9a.size());
}

TEST(TextLine, GetIndentAndIndentStr) {
  std::wstring indent_str;
  std::wstring str = L"abc";

  TextLine line(0, L"");
  EXPECT_EQ(0, line.GetIndent(4));
  EXPECT_EQ(L"", line.GetIndentStr());

  line.Clear();
  line.InsertString(0, str);
  EXPECT_EQ(0, line.GetIndent(4));
  EXPECT_EQ(L"", line.GetIndentStr());

  indent_str = L"   ";
  line.Clear();
  line.InsertString(0, indent_str);
  EXPECT_EQ(3, line.GetIndent(4));
  EXPECT_EQ(indent_str, line.GetIndentStr());

  indent_str = L"   ";
  line.Clear();
  line.InsertString(0, indent_str + str);
  EXPECT_EQ(3, line.GetIndent(4));
  EXPECT_EQ(indent_str, line.GetIndentStr());

  indent_str = L"\t";
  line.Clear();
  line.InsertString(0, indent_str + str);
  EXPECT_EQ(4, line.GetIndent(4));
  EXPECT_EQ(indent_str, line.GetIndentStr());

  indent_str = L"\t\t";
  line.Clear();
  line.InsertString(0, indent_str + str);
  EXPECT_EQ(8, line.GetIndent(4));
  EXPECT_EQ(indent_str, line.GetIndentStr());

  indent_str = L"  \t\t";
  line.Clear();
  line.InsertString(0, indent_str + str);
  EXPECT_EQ(8, line.GetIndent(4));
  EXPECT_EQ(indent_str, line.GetIndentStr());

  indent_str = L"  \t  \t";
  line.Clear();
  line.InsertString(0, indent_str + str);
  EXPECT_EQ(8, line.GetIndent(4));
  EXPECT_EQ(L"  \t  \t", line.GetIndentStr());

  indent_str = L"    \t  \t";
  line.Clear();
  line.InsertString(0, indent_str + str);
  EXPECT_EQ(12, line.GetIndent(4));
  EXPECT_EQ(indent_str, line.GetIndentStr());
}

