#ifndef UI_BUTTON_BASE_H_
#define UI_BUTTON_BASE_H_

#include "wx/control.h"

#include "ui/button_style.h"

namespace ui {

class ButtonBase : public wxControl {
  DECLARE_CLASS(ButtonBase)
  DECLARE_NO_COPY_CLASS(ButtonBase)
  DECLARE_EVENT_TABLE()

public:
  // Click type determines if the click event will be posted on
  // mouse left down or mouse left up (default).
  enum ClickType {
    kClickOnUp,
    kClickOnDown
  };

public:
  explicit ButtonBase(SharedButtonStyle style);
  virtual ~ButtonBase();

  bool Create(wxWindow* parent, wxWindowID id);

  bool Enable(bool enable) override;

  bool AcceptsFocus() const override {
    return accepts_focus_;
  }
  bool AcceptsFocusFromKeyboard() const override {
    return accepts_focus_;
  }

  void set_accepts_focus(bool accepts_focus) {
    accepts_focus_ = accepts_focus;
  }

  void set_click_type(ClickType click_type) {
    click_type_ = click_type;
  }

  void set_padding(const wxSize& padding) {
    padding_ = padding;
  }

  void set_min_size(const wxSize& min_size) {
    min_size_ = min_size;
  }

  void set_draw_bg(bool draw_bg) {
    draw_bg_ = draw_bg;
  }

  void set_draw_border(bool draw_border) {
    draw_border_ = draw_border;
  }

protected:
  void InitPadding();

  virtual ButtonStyle::State GetState() const = 0;

  void DrawBg(wxDC& dc, ButtonStyle::State state);
  virtual void DrawFg(wxDC& dc, ButtonStyle::State state) = 0;

  virtual void PostEvent() = 0;

  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseLeftDClick(wxMouseEvent& evt);
  void OnMouseEnter(wxMouseEvent& evt);
  void OnMouseLeave(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);
  void OnKeyUp(wxKeyEvent& evt);

protected:
  SharedButtonStyle style_;

  bool accepts_focus_;

  ClickType click_type_;

  bool hover_;
  bool pressed_;

  wxSize padding_;
  wxSize min_size_;

  bool draw_bg_;
  bool draw_border_;
};

}  // namespace ui

#endif  // UI_BUTTON_BASE_H_