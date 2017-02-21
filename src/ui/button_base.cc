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
EVT_LEFT_DCLICK         (ButtonBase::OnMouseLeftDClick)
EVT_ENTER_WINDOW        (ButtonBase::OnMouseEnter)
EVT_LEAVE_WINDOW        (ButtonBase::OnMouseLeave)
EVT_MOUSE_CAPTURE_LOST  (ButtonBase::OnMouseCaptureLost)
EVT_KEY_UP              (ButtonBase::OnKeyUp)
EVT_SET_FOCUS           (ButtonBase::OnSetFocus)
EVT_KILL_FOCUS          (ButtonBase::OnKillFocus)
END_EVENT_TABLE()

ButtonBase::ButtonBase(SharedButtonStyle style)
    : style_(style)
    , accepts_focus_(true)
    , click_type_(kClickOnUp)
    , hover_(false)
    , pressed_(false)
    , user_best_size_(wxDefaultSize)
    , draw_bg_(true)
    , draw_border_(true) {
}

ButtonBase::~ButtonBase() {
}

bool ButtonBase::Create(wxWindow* parent, wxWindowID id) {
  if (!wxControl::Create(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)) {
    return false;
  }

  SetExtraStyle(GetExtraStyle() | wxWS_EX_PROCESS_UI_UPDATES);

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(parent->GetBackgroundColour());

  InitPadding();

  SetCursor(wxCURSOR_HAND);

  return true;
}

bool ButtonBase::Enable(bool enable) {
  if (wxControl::Enable(enable)) {
    Refresh();
    return true;
  }
  return false;
}

void ButtonBase::InitPadding() {
  // Determine padding by char width.
  int cw = GetCharWidth();
  padding_.Set(cw, cw / 2);
}

void ButtonBase::DrawBg(wxDC& dc, ButtonStyle::State state) {
  if (!draw_border_ && !draw_bg_) {
    return;
  }

  if (draw_border_) {
    wxColour border = style_->GetColor(ButtonStyle::BORDER, state);
    dc.SetPen(wxPen(border));
  } else {
    dc.SetPen(*wxTRANSPARENT_PEN);
  }

  if (draw_bg_) {
    wxColour bg = style_->GetColor(ButtonStyle::BG, state);
    dc.SetBrush(wxBrush(bg));
  } else {
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
  }

  dc.DrawRectangle(GetClientRect());
}

void ButtonBase::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
#endif

  ButtonStyle::State state = GetState();
  DrawBg(dc, state);
  DrawFg(dc, state);
}

void ButtonBase::OnMouseLeftDown(wxMouseEvent& evt) {
  pressed_ = true;

  if (click_type_ == kClickOnUp) {
    if (!HasCapture()) {
      CaptureMouse();
    }
  } else {
    PostEvent();
  }

  Refresh();
  evt.Skip();
}

void ButtonBase::OnMouseLeftUp(wxMouseEvent& evt) {
  if (HasCapture()) {
    ReleaseMouse();
  }

  if (pressed_) {
    if (click_type_ == kClickOnUp) {
      if (GetClientRect().Contains(evt.GetPosition())) {
        PostEvent();
      }
    }
  }

  pressed_ = false;
  Refresh();

  evt.Skip();
}

// NOTE:
// If you double click this control, the mouse events would be:
//   - Left Down
//   - Left Up
//   - Left DClick
//   - Left Up
// There won't be two Left Down, instead the second Left Down is replaced
// by Left DClick.
void ButtonBase::OnMouseLeftDClick(wxMouseEvent& evt) {
  pressed_ = true;

  // NOTE: Don't capture mouse.
  if (click_type_ == kClickOnDown) {
    PostEvent();
  }

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
  pressed_ = false;
  Refresh();
  evt.Skip();
}

}  // namespace ui
}  // namespace jil
