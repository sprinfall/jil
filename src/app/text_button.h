#ifndef CSUI_TEXT_BUTTON_H_
#define CSUI_TEXT_BUTTON_H_
#pragma once

// A button displaying text label.

#include "wx/control.h"
#include "boost/shared_ptr.hpp"

namespace jil {\

class TextButton : public wxControl {
  DECLARE_CLASS(TextButton)
  DECLARE_NO_COPY_CLASS(TextButton)
  DECLARE_EVENT_TABLE()

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

  class Style {
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

  typedef boost::shared_ptr<Style> SharedStyle;

public:
  explicit TextButton(SharedStyle style);
  virtual ~TextButton();

  bool Create(wxWindow* parent, wxWindowID id, const wxString& label);

  virtual bool AcceptsFocus() const override {
    return accepts_focus_;
  }

  void set_accepts_focus(bool accepts_focus) {
    accepts_focus_ = accepts_focus;
  }

#if defined (__WXMSW__)
  virtual bool MSWCommand(WXUINT param, WXWORD id) override;
#endif

  void SetColors(const wxColor& bg_normal,
                 const wxColor& bg_select,
                 const wxColor& fg);

protected:
  virtual wxSize DoGetBestSize() const override;

  void PostEvent();

  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseEnter(wxMouseEvent& evt);
  void OnMouseLeave(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);
  void OnKeyUp(wxKeyEvent& evt);

private:
  SharedStyle style_;

  bool pressed_;
  bool hover_;

  bool accepts_focus_;
};

}  // namespace jil

#endif  // CSUI_TEXT_BUTTON_H_