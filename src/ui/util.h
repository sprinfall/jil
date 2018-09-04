#ifndef UI_UTIL_H_
#define UI_UTIL_H_

#include "wx/gdicmn.h"
#include "wx/string.h"

class wxBitmap;
class wxDC;

namespace ui {

extern const wxString kEllipsis;  // ...

size_t TailorLabel(const wxDC& dc, const wxString& label, int max_width);

// Draw text in the rect.
// The text will be truncated and ellipsis will be appended if it's too long.
void DrawTextInRect(wxDC& dc, const wxString& text, const wxRect& rect);

// Get the size of the given bitmap.
// Return wxDefaultSize if the bitmap is not OK.
wxSize GetBitmapSize(const wxBitmap& bitmap);

}  // namespace ui

#endif  // UI_UTIL_H_
