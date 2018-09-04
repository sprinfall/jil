#include <iostream>
#include <set>

#include "gtest/gtest.h"

#include "wx/gdicmn.h"
#include "wx/font.h"
#include "wx/fontenum.h"

#include "editor/text_extent.h"
#include "editor/wrap.h"

using namespace editor;

class FontEnumerator : public wxFontEnumerator {
public:
  virtual bool OnFacename(const wxString& facename) override {
    facename_ = facename;
    return false;
  }

  wxString facename_;
};

static
wxFont GetMonospaceFont() {
  FontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);

  wxFont* font = wxTheFontList->FindOrCreateFont(
      10,
      wxFONTFAMILY_DEFAULT,
      wxFONTSTYLE_NORMAL,
      wxFONTWEIGHT_NORMAL,
      false,
      fe.facename_);

  return *font;
}

class WrapTest : public testing::Test {
 protected:
  virtual void SetUp() {
    text_extent_.SetFont(GetMonospaceFont());
    char_width_ = text_extent_.GetWidth(_T("T"));
  }

  // virtual void TearDown() will be called after each test is run.
  // You should define it if there is cleanup work to do.  Otherwise,
  // you don't have to provide it.
  //
  // virtual void TearDown() {
  // }

  TextExtent text_extent_;
  int char_width_;
};

/*
TEST_F(WrapTest, Test1) {
  std::wstring line(L"ABCDEFG");
  const int client_width = 3 * char_width_;
  WrapOffsets offsets = WrapLineByChar(text_extent_, line, client_width);

  EXPECT_EQ(2, offsets.size());
  EXPECT_EQ(3, offsets[0]);
  EXPECT_EQ(6, offsets[1]);
}
TEST(WrapLine, Test2) {
  TextExtent text_extent;
  wxString line(_T("ABCDEFG"));
  const int client_width = 2 * kCharWidth + kCharWidth / 2;
  Wraps wraps = WrapLine(text_extent, line, client_width);

  EXPECT_EQ(3, wraps.size());
  EXPECT_EQ(2, wraps[0]);
  EXPECT_EQ(4, wraps[1]);
  EXPECT_EQ(6, wraps[2]);
}

TEST(WrapLine, Test3) {
  TextExtent text_extent;
  wxString line(_T("ABCDEFG"));
  const int client_width = 3 * kCharWidth + kCharWidth / 2;
  Wraps wraps = WrapLine(text_extent, line, client_width);

  EXPECT_EQ(2, wraps.size());
  EXPECT_EQ(3, wraps[0]);
  EXPECT_EQ(6, wraps[1]);
}

TEST(WrapLine, Test4) {
  TextExtent text_extent;
  wxString line(_T("ABCDEFGHI"));
  const int client_width = 3 * kCharWidth;
  Wraps wraps = WrapLine(text_extent, line, client_width);

  EXPECT_EQ(2, wraps.size());
  EXPECT_EQ(3, wraps[0]);
  EXPECT_EQ(6, wraps[1]);
}

TEST(WrapLine, Test5) {
  TextExtent text_extent;
  wxString line(_T("ABCDEFGHI"));
  const int client_width = 3 * kCharWidth + kCharWidth / 2;
  Wraps wraps = WrapLine(text_extent, line, client_width);

  EXPECT_EQ(2, wraps.size());
  EXPECT_EQ(3, wraps[0]);
  EXPECT_EQ(6, wraps[1]);
}

TEST(WrapLine, Test6) {
  TextExtent text_extent;
  wxString line(_T("ABC"));
  const int client_width = 3 * kCharWidth;
  Wraps wraps = WrapLine(text_extent, line, client_width);

  EXPECT_EQ(0, wraps.size());
}

TEST(WrapLine, Test7) {
  TextExtent text_extent;
  wxString line(_T("AB"));
  const int client_width = 3 * kCharWidth;
  Wraps wraps = WrapLine(text_extent, line, client_width);

  EXPECT_EQ(0, wraps.size());
}*/