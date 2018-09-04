#ifndef UI_TEXT_BUTTON_H_
#define UI_TEXT_BUTTON_H_

// A button displaying text label.

#include "ui/button_base.h"

namespace ui {

class TextButton : public ButtonBase {
  DECLARE_CLASS(TextButton)
  DECLARE_NO_COPY_CLASS(TextButton)

public:
  explicit TextButton(SharedButtonStyle style) : ButtonBase(style) {
  }

  ~TextButton() override = default;

  bool Create(wxWindow* parent, wxWindowID id, const wxString& label);

protected:
  wxSize DoGetBestSize() const override;

  ButtonStyle::State GetState() const override;

  void DrawFg(wxDC& dc, ButtonStyle::State state) override;

  void PostEvent() override;
};

}  // namespace ui

#endif  // UI_TEXT_BUTTON_H_
