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

}  // namespace ui
}  // namespace jil
