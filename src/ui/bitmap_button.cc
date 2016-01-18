#include "ui/bitmap_button.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(BitmapButton, ButtonBase);

BitmapButton::BitmapButton(SharedButtonStyle style)
    : ButtonBase(style) {
}

BitmapButton::~BitmapButton() {
}

bool BitmapButton::Create(wxWindow* parent, wxWindowID id) {
  if (!ButtonBase::Create(parent, id, wxEmptyString)) {
    return false;
  }

  return true;
}

wxSize BitmapButton::DoGetBestSize() const {
  if (user_best_size_ != wxDefaultSize) {
    return user_best_size_;
  }

  if (bitmap_.IsOk()) {
    return wxSize(bitmap_.GetWidth(), bitmap_.GetHeight());
  }

  return wxDefaultSize;
}

ButtonStyle::State BitmapButton::GetState() const {
  if (!IsEnabled()) {
    return ButtonStyle::DISABLED;
  }

  if (pressed_) {
    return hover_ ? ButtonStyle::PRESSED_HOVER : ButtonStyle::PRESSED;
  } else {
    return hover_ ? ButtonStyle::NORMAL_HOVER : ButtonStyle::NORMAL;
  }
}

void BitmapButton::DrawForeground(wxDC& dc, ButtonStyle::State state) {
  if (bitmap_.IsOk()) {
    wxRect rect = GetClientRect();
    int x = (rect.GetWidth() - bitmap_.GetWidth()) / 2;
    int y = (rect.GetHeight() - bitmap_.GetHeight()) / 2;
    dc.DrawBitmap(bitmap_, x, y, true);
  }
}

void BitmapButton::PostEvent() {
  wxCommandEvent evt(wxEVT_BUTTON, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace ui
}  // namespace jil
