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

  wxColour bg = style_->GetColor(ButtonStyle::BG, state);
  wxColour border = style_->GetColor(ButtonStyle::BORDER, state);

  if (!bg.IsOk()) {
    bg = GetBackgroundColour();
  }

  wxRect rect = GetClientRect();

  int color_delta = (rect.GetHeight() + 1) / 2;
  wxColour bg_top = ui::IncColor(bg, color_delta);
  dc.GradientFillLinear(rect, bg, bg_top, wxNORTH);

  dc.SetPen(wxPen(border));
  dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.DrawRectangle(rect);

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
