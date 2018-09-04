#ifndef UI_BITMAP_BUTTON_H_
#define UI_BITMAP_BUTTON_H_

// A replacement of wxBitmapButton.
// Note:
// BitmapButton emits the same event as wxBitmapButton:
//   Event type: wxEVT_BUTTON
//   Macro: EVT_BUTTON(id, func)

#include "ui/bitmap_button_base.h"

#include "wx/button.h"  // For wxEVT_BUTTON, etc.

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

  void PostEvent() override;
};

}  // namespace ui

#endif  // UI_BITMAP_BUTTON_H_
