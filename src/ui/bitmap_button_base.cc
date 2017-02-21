#include "ui/bitmap_button_base.h"
#include "wx/dcbuffer.h"
#include "ui/util.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(BitmapButtonBase, ButtonBase);

BitmapButtonBase::BitmapButtonBase(SharedButtonStyle style)
    : ButtonBase(style) {
}

BitmapButtonBase::~BitmapButtonBase() {
}

bool BitmapButtonBase::Create(wxWindow* parent, wxWindowID id) {
  if (!ButtonBase::Create(parent, id)) {
    return false;
  }

  return true;
}

void BitmapButtonBase::SetBitmaps(const wxBitmap& normal,
                                  const wxBitmap& hover,
                                  const wxBitmap& pressed,
                                  const wxBitmap& disabled) {
  bitmaps_[ButtonStyle::NORMAL] = normal;
  bitmaps_[ButtonStyle::HOVER] = hover;
  bitmaps_[ButtonStyle::PRESSED] = pressed;
  bitmaps_[ButtonStyle::DISABLED] = disabled;
}

void BitmapButtonBase::SetBitmaps(const wxBitmap& bitmap) {
  SetBitmaps(bitmap, bitmap, bitmap, bitmap);
}

void BitmapButtonBase::SetBitmapsND(const wxBitmap& normal, const wxBitmap& disabled) {
  SetBitmaps(normal, normal, normal, disabled);
}

wxSize BitmapButtonBase::DoGetBestSize() const {
  if (user_best_size_ != wxDefaultSize) {
    return user_best_size_;
  }
  return GetBitmapSize(bitmaps_[ButtonStyle::NORMAL]);
}

void BitmapButtonBase::DrawFg(wxDC& dc, ButtonStyle::State state) {
  const wxBitmap& bitmap = bitmaps_[state];
  if (bitmap.IsOk()) {
    wxRect rect = GetClientRect();
    int x = (rect.GetWidth() - bitmap.GetWidth()) / 2;
    int y = (rect.GetHeight() - bitmap.GetHeight()) / 2;
    dc.DrawBitmap(bitmap, x, y, true);
  }
}

}  // namespace ui
}  // namespace jil
