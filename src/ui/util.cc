#include "ui/util.h"
#include "wx/dc.h"

namespace jil {
namespace ui {

const wxString kEllipsis = wxT("...");

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

void DrawTextInRect(wxDC& dc, const wxString& text, const wxRect& rect) {
  if (text.size() <= 3) {
    dc.DrawText(text, rect.x, rect.y);
    return;
  }

  int w = 0;
  dc.GetTextExtent(text, &w, NULL);
  if (w <= rect.width) {
    dc.DrawText(text, rect.x, rect.y);
    return;
  }

  int ellipsis_w = 0;
  dc.GetTextExtent(kEllipsis, &ellipsis_w, NULL);

  size_t i = TailorLabel(dc, text, rect.width - ellipsis_w);
  dc.DrawText(text.Mid(0, i) + kEllipsis, rect.x, rect.y);
}

wxSize GetBitmapSize(const wxBitmap& bitmap) {
  if (!bitmap.IsOk()) {
    return wxDefaultSize;
  }
  return wxSize(bitmap.GetWidth(), bitmap.GetHeight());
}

}  // namespace ui
}  // namespace jil
