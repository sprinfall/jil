#include "ui/bitmap_button.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(BitmapButton, BitmapButtonBase);

BitmapButton::BitmapButton(SharedButtonStyle style)
    : BitmapButtonBase(style) {
}

BitmapButton::~BitmapButton() {
}

bool BitmapButton::Create(wxWindow* parent, wxWindowID id) {
  if (!BitmapButtonBase::Create(parent, id)) {
    return false;
  }

  return true;
}

ButtonStyle::State BitmapButton::GetState() const {
  if (!IsThisEnabled()) {  // NOTE: Don't use IsEnabled()!
    return ButtonStyle::DISABLED;
  }

  if (pressed_) {
    return ButtonStyle::PRESSED;
  } else if (hover_) {
    return ButtonStyle::HOVER;
  } else {
    return ButtonStyle::NORMAL;
  }
}

void BitmapButton::PostEvent() {
  wxCommandEvent evt(wxEVT_BUTTON, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace ui
}  // namespace jil
