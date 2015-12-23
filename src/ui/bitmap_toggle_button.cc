#include "ui/bitmap_toggle_button.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(BitmapToggleButton, ButtonBase);

BitmapToggleButton::BitmapToggleButton(SharedButtonStyle style)
    : ButtonBase(style), toggle_(false) {
}

BitmapToggleButton::~BitmapToggleButton() {
}

bool BitmapToggleButton::Create(wxWindow* parent, wxWindowID id) {
  if (!ButtonBase::Create(parent, id, wxEmptyString)) {
    return false;
  }

  return true;
}

wxSize BitmapToggleButton::DoGetBestSize() const {
  if (user_best_size_ != wxDefaultSize) {
    return user_best_size_;
  }

  if (bitmap_.IsOk()) {
    return wxSize(bitmap_.GetWidth(), bitmap_.GetHeight());
  }

  return wxDefaultSize;
}

ButtonStyle::State BitmapToggleButton::GetState() const {
  if (!IsEnabled()) {
    return ButtonStyle::DISABLED;
  }

  if (toggle_ || pressed_) {
    return hover_ ? ButtonStyle::PRESSED_HOVER : ButtonStyle::PRESSED;
  } else {
    return hover_ ? ButtonStyle::NORMAL_HOVER : ButtonStyle::NORMAL;
  }
}

void BitmapToggleButton::DrawForeground(wxDC& dc, ButtonStyle::State state) {
  if (bitmap_.IsOk()) {
    wxRect rect = GetClientRect();
    int x = (rect.GetWidth() - bitmap_.GetWidth()) / 2;
    int y = (rect.GetHeight() - bitmap_.GetHeight()) / 2;
    dc.DrawBitmap(bitmap_, x, y, true);
  }
}

void BitmapToggleButton::PostEvent() {
  toggle_ = !toggle_;

  wxCommandEvent toggle_evt(wxEVT_TOGGLEBUTTON, GetId());
  toggle_evt.SetEventObject(this);
  // Note: SetInt() affects IsChecked().
  toggle_evt.SetInt(toggle_ ? 1 : 0);
  GetParent()->GetEventHandler()->AddPendingEvent(toggle_evt);
}

}  // namespace ui
}  // namespace jil
