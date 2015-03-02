#ifndef JIL_UI_TEXT_BUTTON_H_
#define JIL_UI_TEXT_BUTTON_H_
#pragma once

// A button displaying text label.

#include "wx/control.h"
#include "boost/shared_ptr.hpp"

namespace jil {
namespace ui {

class ButtonStyle {
public:
  enum Part {
    BG = 0,
    FG,
    BORDER,
    PARTS,
  };

  enum State {
    NORMAL = 0,
    HOVER,
    PRESSED,
    DISABLED,
    STATES,
  };

public:
  const wxColour& GetColor(int part, int state) const {
    assert(part >= 0 && part < PARTS);
    assert(state >= 0 && state < STATES);
    return colors_[part][state];
  }

  void SetColor(int part, int state, const wxColour& color) {
    assert(part >= 0 && part < PARTS);
    assert(state >= 0 && state < STATES);
    colors_[part][state] = color;
  }

private:
  wxColor colors_[PARTS][STATES];
};

typedef boost::shared_ptr<ButtonStyle> SharedButtonStyle;

class TextButton : public wxControl {
  DECLARE_CLASS(TextButton)
  DECLARE_NO_COPY_CLASS(TextButton)
  DECLARE_EVENT_TABLE()

public:
  explicit TextButton(SharedButtonStyle style);
  virtual ~TextButton();

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
  virtual wxSize DoGetBestSize() const override;

  virtual void PostEvent();

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

#endif  // JIL_UI_TEXT_BUTTON_H_