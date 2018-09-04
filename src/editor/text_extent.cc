#include "editor/text_extent.h"

#include "wx/dcmemory.h"
#include "wx/font.h"

#ifdef __WXMSW__
#include "wx/msw/private.h"
#endif  // __WXMSW__

#include "editor/tab.h"
#include "editor/text_line.h"

namespace editor {

TextExtent::TextExtent() : dc_(NULL) {
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

wxCoord TextExtent::GetWidth(const std::wstring& text) const {
  wxCoord x = 0;
  GetExtent(text, 0, text.length(), &x, NULL);
  return x;
}

wxCoord TextExtent::GetWidth(const std::wstring& text, Coord off, Coord len) const {
  wxCoord x = 0;
  GetExtent(text, off, len, &x, NULL);
  return x;
}

// TODO: If the font is fixed-width and no CJK charactors,
// use expanded_length * char_size_.x directory.
wxCoord TextExtent::GetLineWidth(int tab_stop,
                                 const TextLine* line,
                                 Coord off,
                                 Coord len,
                                 Coord base) const {
  if (!line->HasTabs()) {
    return GetWidth(line->data(), base + off, len);
  }

  wxCoord width = 0;
  Coord expanded_length = 0;

  Coord end = base + off + len;

  for (Coord i = base; i < base + off; ++i) {
    if (line->Char(i) == kTabChar) {
      expanded_length += tab_stop - (expanded_length % tab_stop);
    } else {
      ++expanded_length;
    }
  }

  Coord p = base + off;

  for (Coord i = base + off; i < end; ++i) {
    if (line->Char(i) == kTabChar) {
      if (i > p) {
        width += GetWidth(line->data(), p, i - p);
      }

      int chars = tab_stop - (expanded_length % tab_stop);
      width += char_size_.x * chars;
      expanded_length += chars;

      p = i + 1;
    } else {
      ++expanded_length;
    }
  }

  if (p < end) {
    width += GetWidth(line->data(), p, end - p);
  }

  return width;
}

Coord TextExtent::IndexChar(int tab_stop,
                            const TextLine* line,
                            Coord base,
                            int client_x,
                            bool vspace) {
  if (base >= line->Length()) {
    if (!vspace) {  // Virtual space not allowed.
      return 0;
    }

    Coord chars = client_x / char_size_.x;
    if ((client_x % char_size_.x) > (char_size_.x / 2)) {
      ++chars;
    }
    return chars;
  }

  return IndexCharRecursively(tab_stop, line, base, 0, line->Length() - base, client_x, vspace);
}

void TextExtent::UpdateCharSize() {
  char_size_.x = dc_->GetCharWidth();
  char_size_.y = dc_->GetCharHeight();
}

void TextExtent::GetExtent(const std::wstring& text,
                           Coord off,
                           Coord len,
                           wxCoord* x,
                           wxCoord* y,
                           wxCoord* external_leading) const {
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
                                       const TextLine* line,
                                       Coord base,
                                       Coord begin,
                                       Coord end,
                                       int client_x,
                                       bool vspace) const {
  if (begin >= end) {
    return begin;
  }

  Coord m = begin + (end - begin) / 2;
  int width = GetLineWidth(tab_stop, line, 0, m, base);

  if (abs(width - client_x) < (char_size_.x / 2)) {
    return m;
  } else if (client_x > width) {
    if (vspace && m + 1 >= end) {  // No more chars.
      Coord chars = (client_x - width) / char_size_.x;
      return m + chars;
    } else {
      return IndexCharRecursively(tab_stop, line, base, m + 1, end, client_x, vspace);
    }
  } else {  // client_x < width
    return IndexCharRecursively(tab_stop, line, base, begin, m, client_x, vspace);
  }
}

}  // namespace editor
