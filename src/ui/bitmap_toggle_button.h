#ifndef JIL_UI_BITMAP_TOGGLE_BUTTON_H_
#define JIL_UI_BITMAP_TOGGLE_BUTTON_H_
#pragma once

// A replacement of wxBitmapToggleButton.

#include "wx/control.h"
#include "wx/tglbtn.h"  // For wxEVT_TOGGLEBUTTON, etc.
#include "wx/bitmap.h"

namespace jil {
namespace ui {

// Note:
// BitmapToggleButton emits the same event as wxBitmapToggleButton:
//   Event type: wxEVT_TOGGLEBUTTON
//   Macro: EVT_TOGGLEBUTTON(id, func)

class BitmapToggleButton : public wxControl {
  DECLARE_EVENT_TABLE()

 public:
  BitmapToggleButton(wxWindow* parent, wxWindowID id);
  virtual ~BitmapToggleButton();

  virtual bool Enable(bool enable) override;

  virtual bool AcceptsFocus() const override {
    return accept_focus_;
  }

  void set_accept_focus(bool accept_focus) {
    accept_focus_ = accept_focus;
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

  void SetBitmaps(const wxBitmap& bitmap,
                  const wxBitmap& toggle_bitmap,
                  const wxBitmap& disabled_bitmap = wxNullBitmap,
                  const wxBitmap& disabled_toggle_bitmap = wxNullBitmap) {
    bitmap_ = bitmap;
    toggle_bitmap_ = toggle_bitmap;
    disabled_bitmap_ = disabled_bitmap;
    disabled_toggle_bitmap_ = disabled_toggle_bitmap;
  }

 protected:
  virtual wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);

  void OnLeftDown(wxMouseEvent& evt);
  void OnLeftUp(wxMouseEvent& evt);

 private:
  bool accept_focus_;
  bool toggle_;

  wxBitmap bitmap_;
  wxBitmap toggle_bitmap_;
  wxBitmap disabled_bitmap_;
  wxBitmap disabled_toggle_bitmap_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BITMAP_TOGGLE_BUTTON_H_
