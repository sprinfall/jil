#include "ui/bitmap_toggle_button.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(BitmapToggleButton, BitmapButtonBase);

BitmapToggleButton::BitmapToggleButton(SharedButtonStyle style)
    : BitmapButtonBase(style), toggle_(false), auto_switch_(true) {
}

BitmapToggleButton::~BitmapToggleButton() {
}

bool BitmapToggleButton::Create(wxWindow* parent, wxWindowID id) {
  if (!BitmapButtonBase::Create(parent, id)) {
    return false;
  }

  return true;
}

ButtonStyle::State BitmapToggleButton::GetState() const {
  if (!IsThisEnabled()) {  // NOTE: Don't use IsEnabled()!
    return ButtonStyle::DISABLED;
  }

  if (toggle_ || pressed_) {
    return ButtonStyle::PRESSED;
  } else if (hover_) {
    return ButtonStyle::HOVER;
  } else {
    return ButtonStyle::NORMAL;
  }
}

void BitmapToggleButton::PostEvent() {
  if (auto_switch_) {
    toggle_ = !toggle_;
  }

  wxCommandEvent toggle_evt(wxEVT_TOGGLEBUTTON, GetId());
  toggle_evt.SetEventObject(this);
  toggle_evt.SetInt(toggle_ ? 1 : 0);  // Note: SetInt() affects IsChecked().
  GetParent()->GetEventHandler()->AddPendingEvent(toggle_evt);
}

}  // namespace ui
}  // namespace jil
