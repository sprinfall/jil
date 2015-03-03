#ifndef JIL_UI_BITMAP_TOGGLE_BUTTON_H_
#define JIL_UI_BITMAP_TOGGLE_BUTTON_H_
#pragma once

// A replacement of wxBitmapToggleButton.

#include "ui/button_base.h"
#include "wx/tglbtn.h"  // For wxEVT_TOGGLEBUTTON, etc.
#include "wx/bitmap.h"

namespace jil {
namespace ui {

// Note:
// BitmapToggleButton emits the same event as wxBitmapToggleButton:
//   Event type: wxEVT_TOGGLEBUTTON
//   Macro: EVT_TOGGLEBUTTON(id, func)

class BitmapToggleButton : public ButtonBase {
  DECLARE_CLASS(BitmapToggleButton)
  DECLARE_NO_COPY_CLASS(BitmapToggleButton)

public:
  explicit BitmapToggleButton(SharedButtonStyle style);
  virtual ~BitmapToggleButton();

  bool Create(wxWindow* parent, wxWindowID id);

  void set_user_best_size(const wxSize& user_best_size) {
    user_best_size_ = user_best_size;
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

  void SetBitmap(const wxBitmap& bitmap) {
    bitmap_ = bitmap;
  }

protected:
  virtual wxSize DoGetBestSize() const override;

  virtual ButtonStyle::State GetState() override;

  virtual void DrawForeground(wxDC& dc, ButtonStyle::State state) override;

  virtual void PostEvent() override;

private:
  wxSize user_best_size_;
  bool toggle_;
  wxBitmap bitmap_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BITMAP_TOGGLE_BUTTON_H_
