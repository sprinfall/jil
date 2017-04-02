#ifndef JIL_UI_BITMAP_BUTTON_H_
#define JIL_UI_BITMAP_BUTTON_H_
#pragma once

// A replacement of wxBitmapButton.
// Note:
// BitmapButton emits the same event as wxBitmapButton:
//   Event type: wxEVT_BUTTON
//   Macro: EVT_BUTTON(id, func)

#include "ui/bitmap_button_base.h"
#include "wx/button.h" // For wxEVT_BUTTON, etc.

namespace jil {
namespace ui {

class BitmapButton : public BitmapButtonBase {
  DECLARE_CLASS(BitmapButton)
  DECLARE_NO_COPY_CLASS(BitmapButton)

public:
  explicit BitmapButton(SharedButtonStyle style);
  virtual ~BitmapButton();

  bool Create(wxWindow* parent, wxWindowID id);

protected:
  ButtonStyle::State GetState() const override;

  virtual void PostEvent() override;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BITMAP_BUTTON_H_
