#include "ui/button_base.h"
#include "wx/dcbuffer.h"
#include "ui/color.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(ButtonBase, wxControl);

BEGIN_EVENT_TABLE(ButtonBase, wxControl)
EVT_PAINT               (ButtonBase::OnPaint)
EVT_LEFT_DOWN           (ButtonBase::OnMouseLeftDown)
EVT_LEFT_UP             (ButtonBase::OnMouseLeftUp)
EVT_ENTER_WINDOW        (ButtonBase::OnMouseEnter)
EVT_LEAVE_WINDOW        (ButtonBase::OnMouseLeave)
EVT_MOUSE_CAPTURE_LOST  (ButtonBase::OnMouseCaptureLost)
EVT_KEY_UP              (ButtonBase::OnKeyUp)
EVT_SET_FOCUS           (ButtonBase::OnSetFocus)
EVT_KILL_FOCUS          (ButtonBase::OnKillFocus)
END_EVENT_TABLE()

ButtonBase::ButtonBase(SharedButtonStyle style)
    : style_(style)
    , padding_(10, 7)
    , pressed_(false)
    , hover_(false)
    , accepts_focus_(true) {
}

ButtonBase::~ButtonBase() {
}

bool ButtonBase::Create(wxWindow* parent,
                        wxWindowID id,
                        const wxString& label) {
  assert(style_);

  if (!wxControl::Create(parent,
                         id,
                         wxDefaultPosition,
                         wxDefaultSize,
                         wxBORDER_NONE)) {
    return false;
  }

  SetLabel(label);

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(parent->GetBackgroundColour());

  return true;
}

bool ButtonBase::Enable(bool enable) {
  if (wxControl::Enable(enable)) {
    Refresh();
    return true;
  }
  return false;
}

void ButtonBase::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if (!wxALWAYS_NATIVE_DOUBLE_BUFFER)
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
#endif

  ButtonStyle::State state = GetState();

  wxColour bg1 = style_->GetColor(ButtonStyle::BG1, state);
  wxColour bg2 = style_->GetColor(ButtonStyle::BG2, state);
  wxColour bg3 = style_->GetColor(ButtonStyle::BG3, state);
  wxColour border_outer = style_->GetColor(ButtonStyle::BORDER_OUTER, state);
  wxColour border_inner = style_->GetColor(ButtonStyle::BORDER_INNER, state);

  const wxRect client_rect = GetClientRect();

  // Background rects
  wxRect inner_rect = client_rect;
  inner_rect.Deflate(2, 2);

  wxRect rect1 = inner_rect;
  rect1.height = inner_rect.height / 2 - 2;

  wxRect rect2 = inner_rect;
  rect2.y = inner_rect.y + rect1.height;
  rect2.height = inner_rect.height - rect1.height - 2;

  wxRect rect3 = inner_rect;
  rect3.y = inner_rect.y + rect1.height + rect2.height;
  rect3.height = 2;

  // Background
  dc.GradientFillLinear(rect1, bg2, bg1, wxNORTH);

  dc.SetPen(wxPen(bg2));
  dc.SetBrush(wxBrush(bg2));
  dc.DrawRectangle(rect2);

  dc.SetPen(wxPen(bg3));
  dc.SetBrush(wxBrush(bg3));
  dc.DrawRectangle(rect3);

  // Borders
  dc.SetBrush(*wxTRANSPARENT_BRUSH);

  wxRect border_rect = client_rect;
  dc.SetPen(wxPen(border_outer));
  dc.DrawRectangle(border_rect);

  border_rect.Deflate(1, 1);
  dc.SetPen(wxPen(border_inner));
  dc.DrawRectangle(border_rect);

  // Draw 2 more inner borders.
  if (state == ButtonStyle::NORMAL_HOVER || state == ButtonStyle::PRESSED_HOVER) {
    int r = border_inner.Red();
    int g = border_inner.Green();
    int b = border_inner.Blue();

    int delta_r = (bg2.Red() - r) / 3;
    int delta_g = (bg2.Green() - g) / 3;
    int delta_b = (bg2.Blue() - b) / 3;

    for (size_t i = 0; i < 2; ++i) {
      r += delta_r;
      g += delta_g;
      b += delta_b;

      border_rect.Deflate(1, 1);
      dc.SetPen(wxPen(wxColour(r, g, b)));
      dc.DrawRectangle(border_rect);
    }
  }

  // Foreground
  DrawForeground(dc, state);
}

void ButtonBase::OnMouseLeftDown(wxMouseEvent& evt) {
  pressed_ = true;

  if (!HasCapture()) {
    CaptureMouse();
  }

  Refresh();
  evt.Skip();
}

void ButtonBase::OnMouseLeftUp(wxMouseEvent& evt) {
  if (GetClientRect().Contains(evt.GetPosition())) {
    PostEvent();
  }

  if (HasCapture()) {
    ReleaseMouse();
  }

  pressed_ = false;
  Refresh();

  evt.Skip();
}

void ButtonBase::OnMouseEnter(wxMouseEvent& evt) {
  hover_ = true;
  Refresh();
  evt.Skip();
}

void ButtonBase::OnMouseLeave(wxMouseEvent& evt) {
  hover_ = false;
  Refresh();
  evt.Skip();
}

void ButtonBase::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  pressed_ = false;
  hover_ = false;
  Refresh();
}

void ButtonBase::OnKeyUp(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_SPACE) {
    PostEvent();
  }
  evt.Skip();
}

void ButtonBase::OnSetFocus(wxFocusEvent& evt) {
  Refresh();
  evt.Skip();
}

void ButtonBase::OnKillFocus(wxFocusEvent& evt) {
  Refresh();
  evt.Skip();
}

}  // namespace ui
}  // namespace jil
