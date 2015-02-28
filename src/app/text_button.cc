#include "app/text_button.h"
#include "wx/dcbuffer.h"
#include "editor/color.h"

namespace jil {

IMPLEMENT_CLASS(TextButton, wxControl);

BEGIN_EVENT_TABLE(TextButton, wxControl)
EVT_PAINT               (TextButton::OnPaint)
EVT_LEFT_DOWN           (TextButton::OnMouseLeftDown)
EVT_LEFT_UP             (TextButton::OnMouseLeftUp)
EVT_ENTER_WINDOW        (TextButton::OnMouseEnter)
EVT_LEAVE_WINDOW        (TextButton::OnMouseLeave)
EVT_MOUSE_CAPTURE_LOST  (TextButton::OnMouseCaptureLost)
EVT_KEY_UP              (TextButton::OnKeyUp)
EVT_SET_FOCUS           (TextButton::OnSetFocus)
EVT_KILL_FOCUS          (TextButton::OnKillFocus)
END_EVENT_TABLE()

TextButton::TextButton(SharedStyle style)
    : style_(style)
    , padding_(10, 7)
    , pressed_(false)
    , hover_(false)
    , accepts_focus_(true) {
}

TextButton::~TextButton() {
}

bool TextButton::Create(wxWindow* parent,
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

wxSize TextButton::DoGetBestSize() const {
  wxSize best_size;
  GetTextExtent(GetLabel(), &best_size.x, &best_size.y);
  best_size.IncBy(padding_.x, padding_.y);

  wxSize min_size = GetMinSize();
  if (best_size.x < min_size.x) {
    best_size.x = min_size.x;
  }
  if (best_size.y < min_size.y) {
    best_size.y = min_size.y;
  }

  return best_size;
}

void TextButton::PostEvent() {
  wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  evt.SetEventObject(this);
  evt.SetString(GetLabel());
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

void TextButton::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if (!wxALWAYS_NATIVE_DOUBLE_BUFFER)
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
#endif

  State state = NORMAL;
  if (IsEnabled()) {
    if (pressed_) {
      state = PRESSED;
    } else if (hover_) {
      state = HOVER;
    } else if (wxWindow::FindFocus() == this) {
      state = HOVER;
    }
  } else {
    state = DISABLED;
  }

  wxColour bg = style_->GetColor(BG, state);
  wxColour fg = style_->GetColor(FG, state);
  wxColour border = style_->GetColor(BORDER, state);

  dc.SetPen(wxPen(border));
  dc.SetBrush(wxBrush(bg));

  wxRect client_rect = GetClientRect();
  dc.DrawRectangle(client_rect);

  dc.SetFont(GetFont());
  dc.SetTextForeground(fg);
  dc.DrawLabel(GetLabel(), client_rect, wxALIGN_CENTER);
}

void TextButton::OnMouseLeftDown(wxMouseEvent& evt) {
  pressed_ = true;

  if (!HasCapture()) {
    CaptureMouse();
  }

  Refresh();
  evt.Skip();
}

void TextButton::OnMouseLeftUp(wxMouseEvent& evt) {
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

void TextButton::OnMouseEnter(wxMouseEvent& evt) {
  hover_ = true;
  Refresh();
  evt.Skip();
}

void TextButton::OnMouseLeave(wxMouseEvent& evt) {
  hover_ = false;
  Refresh();
  evt.Skip();
}

void TextButton::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  pressed_ = false;
  hover_ = false;
  Refresh();
}

void TextButton::OnKeyUp(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_SPACE) {
    PostEvent();
  }
  evt.Skip();
}

void TextButton::OnSetFocus(wxFocusEvent& evt) {
  Refresh();
  evt.Skip();
}

void TextButton::OnKillFocus(wxFocusEvent& evt) {
  Refresh();
  evt.Skip();
}

}  // namespace jil
