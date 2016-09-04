#ifndef JIL_UI_LABEL_H_
#define JIL_UI_LABEL_H_
#pragma once

// A replacement of wxStaticText.
// The difference is that Label emits wxEVT_BUTTON on click.
// So it can be used as button.

#include "wx/control.h"

namespace jil {
namespace ui {

class Label : public wxControl {
  DECLARE_EVENT_TABLE()

public:
  Label(wxWindow* parent, wxWindowID id, const wxString& label);
  virtual ~Label();

  virtual bool AcceptsFocus() const override {
    return accept_focus_;
  }

  void set_accept_focus(bool accept_focus) {
    accept_focus_ = accept_focus;
  }

protected:
  void InitPadding();

  virtual wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);

  void OnLeftDown(wxMouseEvent& evt);
  void OnLeftUp(wxMouseEvent& evt);

private:
  bool accept_focus_;
  wxSize padding_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_LABEL_H_
