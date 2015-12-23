#ifndef JIL_UI_BUTTON_BASE_H_
#define JIL_UI_BUTTON_BASE_H_
#pragma once

#include "wx/control.h"
#include "ui/button_style.h"

namespace jil {
namespace ui {

class ButtonBase : public wxControl {
  DECLARE_CLASS(ButtonBase)
  DECLARE_NO_COPY_CLASS(ButtonBase)
  DECLARE_EVENT_TABLE()

public:
  explicit ButtonBase(SharedButtonStyle style);
  virtual ~ButtonBase();

  bool Create(wxWindow* parent, wxWindowID id, const wxString& label);

  virtual bool Enable(bool enable) override;

  virtual bool AcceptsFocus() const override {
    return accepts_focus_;
  }

  void set_padding(const wxSize& padding) {
    padding_ = padding;
  }

  void set_accepts_focus(bool accepts_focus) {
    accepts_focus_ = accepts_focus;
  }

protected:
  virtual ButtonStyle::State GetState() const = 0;

  virtual void DrawForeground(wxDC& dc, ButtonStyle::State state) = 0;

  virtual void PostEvent() = 0;

  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseEnter(wxMouseEvent& evt);
  void OnMouseLeave(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);
  void OnKeyUp(wxKeyEvent& evt);

protected:
  SharedButtonStyle style_;

  wxSize padding_;

  bool pressed_;
  bool hover_;

  bool accepts_focus_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BUTTON_BASE_H_