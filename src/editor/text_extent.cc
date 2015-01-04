#include "editor/text_extent.h"
#include "wx/dcmemory.h"
#include "wx/font.h"
#include "editor/tab.h"

namespace jil {
namespace editor {

TextExtent::TextExtent()
    : dc_(NULL), char_width_(0) {
  wxBitmap bmp(1, 1);
  dc_ = new wxMemoryDC(bmp);

  char_width_ = dc_->GetCharWidth();
}

TextExtent::~TextExtent() {
  delete dc_;
}

void TextExtent::SetFont(const wxFont& font) {
  dc_->SetFont(font);
  char_width_ = dc_->GetCharWidth();
}

wxCoord TextExtent::GetWidth(const std::wstring& text) {
  wxCoord x = 0;
  GetExtent(text, &x, NULL);
  return x;
}

void TextExtent::GetExtent(const std::wstring& text,
                           wxCoord* x,
                           wxCoord* y,
                           wxCoord* external_leading) {
  dc_->GetTextExtent(wxString(text.c_str()), x, y, 0, external_leading);
}

Coord TextExtent::IndexChar(int tab_stop,
                            const std::wstring& line,
                            int client_x,
                            bool vspace) {
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

  return IndexCharRecursively(tab_stop,
                              line,
                              0,
                              line.length(),
                              client_x,
                              vspace);
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
  int w = 0;
  for (size_t i = 0; i < label.size(); ++i) {
    dc.GetTextExtent(label.Mid(0, i), &w, NULL, NULL, NULL, NULL);
    if (w > max_width) {
      return (i == 0 ? i : i - 1);
    }
  }
  return label.size();
}

}  // namespace editor
}  // namespace jil
