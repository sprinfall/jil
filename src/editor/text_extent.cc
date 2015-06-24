#include "editor/text_extent.h"
#include "wx/dcmemory.h"
#include "wx/font.h"
#ifdef __WXMSW__
#include "wx/msw/private.h"
#endif  // __WXMSW__
#include "editor/tab.h"

namespace jil {
namespace editor {

TextExtent::TextExtent()
    : dc_(NULL), char_width_(0), char_height_(0) {
  wxBitmap bmp(1, 1);
  dc_ = new wxMemoryDC(bmp);
  UpdateCharSize();
}

TextExtent::~TextExtent() {
  wxDELETE(dc_);
}

void TextExtent::SetFont(const wxFont& font) {
  dc_->SetFont(font);
  UpdateCharSize();
}

wxCoord TextExtent::GetWidth(const std::wstring& text) {
  wxCoord x = 0;
  GetExtent(text, 0, text.length(), &x, NULL);
  return x;
}

wxCoord TextExtent::GetWidth(const std::wstring& text, Coord off, Coord len) {
  wxCoord x = 0;
  GetExtent(text, off, len, &x, NULL);
  return x;
}

Coord TextExtent::IndexChar(int tab_stop, const std::wstring& line, int client_x, bool vspace) {
  if (line.empty()) {
    if (!vspace) {  // Virtual space not allowed.
      return 0;
    }

    Coord chars = client_x / char_width_;
    if ((client_x % char_width_) > (char_width_ / 2)) {
      ++chars;
    }
    return chars;
  }

  return IndexCharRecursively(tab_stop, line, 0, line.length(), client_x, vspace);
}

void TextExtent::UpdateCharSize() {
  char_width_ = dc_->GetCharWidth();
  char_height_ = dc_->GetCharHeight();
}

void TextExtent::GetExtent(const std::wstring& text,
                           Coord off,
                           Coord len,
                           wxCoord* x,
                           wxCoord* y,
                           wxCoord* external_leading) {
#ifdef __WXMSW__

  // Use Win32 API to avoid converting std::wstring to wxString.
  HDC hdc = (HDC)dc_->GetHDC();

  SIZE size;
  ::GetTextExtentPoint32W(hdc, text.c_str() + off, len, &size);

  if (x != NULL) {
    *x = size.cx;
  }
  if (y != NULL) {
    *y = size.cy;
  }

#else

  dc_->GetTextExtent(wxString(text.c_str() + off, len), x, y, 0, external_leading);

#endif  // __WXMSW__
}

Coord TextExtent::IndexCharRecursively(int tab_stop,
                                       const std::wstring& line,
                                       Coord begin,
                                       Coord end,
                                       int client_x,
                                       bool vspace) {
  if (begin >= end) {
    return begin;
  }

  Coord m = begin + (end - begin) / 2;

  // TODO: Avoid copy
  // Take unexpanded tab into account.
  std::wstring line_left = line.substr(0, m);
  TabbedLineFast(tab_stop, &line_left);

  int width = GetWidth(line_left);

  // NOTE: Can't use char_width_ because it might be a CJK charactor.
  int m_char_width = GetWidth(std::wstring(1, line[m]));

  if (abs(width - client_x) < (m_char_width / 2)) {
    return m;
  } else if (client_x > width) {
    if (vspace && m + 1 >= end) {  // No more chars.
      Coord chars = (client_x - width) / char_width_;
      return m + chars;
    } else {
      return IndexCharRecursively(tab_stop, line, m + 1, end, client_x, vspace);
    }
  } else {  // client_x < width
    return IndexCharRecursively(tab_stop, line, begin, m, client_x, vspace);
  }
}

////////////////////////////////////////////////////////////////////////////////

size_t TailorLabel(const wxDC& dc, const wxString& label, int max_width) {
  if (label.IsEmpty()) {
    return 0;
  }

  int w = 0;
  size_t i = label.size() - 1;
  for (; i > 0; --i) {
    dc.GetTextExtent(label.Mid(0, i), &w, NULL);
    if (w <= max_width) {
      break;
    }
  }

  return i;
}

}  // namespace editor
}  // namespace jil
