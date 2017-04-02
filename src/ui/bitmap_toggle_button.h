#ifndef JIL_UI_BITMAP_TOGGLE_BUTTON_H_
#define JIL_UI_BITMAP_TOGGLE_BUTTON_H_
#pragma once

// A replacement of wxBitmapToggleButton.

#include "ui/bitmap_button_base.h"
#include "wx/tglbtn.h"  // For wxEVT_TOGGLEBUTTON, etc.

namespace jil {
namespace ui {

// Note:
// BitmapToggleButton emits the same event as wxBitmapToggleButton:
//   Event type: wxEVT_TOGGLEBUTTON
//   Macro: EVT_TOGGLEBUTTON(id, func)

class BitmapToggleButton : public BitmapButtonBase {
  DECLARE_CLASS(BitmapToggleButton)
  DECLARE_NO_COPY_CLASS(BitmapToggleButton)

public:
  explicit BitmapToggleButton(SharedButtonStyle style);
  virtual ~BitmapToggleButton();

  bool Create(wxWindow* parent, wxWindowID id);

  void set_auto_switch(bool auto_switch) {
    auto_switch_ = auto_switch;
  }

  // Switch toggle state.
  void Toggle() {
    toggle_ = !toggle_;
  }

  bool toggle() const {
    return toggle_;
  }
  void set_toggle(bool toggle) {
    toggle_ = toggle;
    Refresh();
  }

  void SetToggleBitmaps(const wxBitmap& normal, const wxBitmap& pressed) {
    SetBitmaps(normal, pressed, pressed, normal);
  }

protected:
  ButtonStyle::State GetState() const override;

  virtual void PostEvent() override;

private:
  bool toggle_;
  bool auto_switch_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BITMAP_TOGGLE_BUTTON_H_
