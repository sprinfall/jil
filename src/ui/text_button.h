#ifndef JIL_UI_TEXT_BUTTON_H_
#define JIL_UI_TEXT_BUTTON_H_
#pragma once

// A button displaying text label.

#include "ui/button_base.h"

namespace jil {
namespace ui {

class TextButton : public ButtonBase {
  DECLARE_CLASS(TextButton)
  DECLARE_NO_COPY_CLASS(TextButton)

public:
  explicit TextButton(SharedButtonStyle style);
  virtual ~TextButton();

  bool Create(wxWindow* parent, wxWindowID id, const wxString& label);

protected:
  virtual wxSize DoGetBestSize() const override;

  virtual ButtonStyle::State GetState() override;

  virtual void DrawForeground(wxDC& dc, ButtonStyle::State state) override;

  virtual void PostEvent() override;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_TEXT_BUTTON_H_