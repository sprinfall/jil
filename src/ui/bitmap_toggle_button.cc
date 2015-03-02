#include "ui/bitmap_toggle_button.h"
#include "wx/dcbuffer.h"

namespace jil {
namespace ui {

BEGIN_EVENT_TABLE(BitmapToggleButton, TextButton)
EVT_PAINT     (BitmapToggleButton::OnPaint)
EVT_LEFT_UP   (BitmapToggleButton::OnMouseLeftUp)
END_EVENT_TABLE()

BitmapToggleButton::BitmapToggleButton(SharedButtonStyle style)
    : TextButton(style), toggle_(false) {
}

BitmapToggleButton::~BitmapToggleButton() {
}

bool BitmapToggleButton::Create(wxWindow* parent, wxWindowID id) {
  if (!TextButton::Create(parent, id, wxEmptyString)) {
    return false;
  }

  return true;
}

wxSize BitmapToggleButton::DoGetBestSize() const {
  if (user_best_size_ != wxDefaultSize) {
    return user_best_size_;
  }

  if (bitmap_.IsOk()) {
    return wxSize(bitmap_.GetWidth(), bitmap_.GetHeight());
  }

  return wxDefaultSize;
}

void BitmapToggleButton::PostEvent() {
  wxCommandEvent toggle_evt(wxEVT_TOGGLEBUTTON, GetId());
  toggle_evt.SetEventObject(this);
  // Note: SetInt() affects IsChecked().
  toggle_evt.SetInt(toggle_ ? 1 : 0);
  GetParent()->GetEventHandler()->AddPendingEvent(toggle_evt);
}

void BitmapToggleButton::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if (!wxALWAYS_NATIVE_DOUBLE_BUFFER)
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
#endif

  ButtonStyle::State state = ButtonStyle::NORMAL;
  if (IsEnabled()) {
    if (toggle_ || pressed_) {
      state = ButtonStyle::PRESSED;
    } else if (hover_) {
      state = ButtonStyle::HOVER;
    } /*else if (wxWindow::FindFocus() == this) {
      state = ButtonStyle::HOVER;
    }*/
  } else {
    state = ButtonStyle::DISABLED;
  }

  wxColour bg = style_->GetColor(ButtonStyle::BG, state);
  wxColour fg = style_->GetColor(ButtonStyle::FG, state);
  wxColour border = style_->GetColor(ButtonStyle::BORDER, state);

  dc.SetPen(wxPen(border));
  dc.SetBrush(wxBrush(bg));

  wxRect client_rect = GetClientRect();
  dc.DrawRectangle(client_rect);

  if (bitmap_.IsOk()) {
    int x = (client_rect.GetWidth() - bitmap_.GetWidth()) / 2;
    int y = (client_rect.GetHeight() - bitmap_.GetHeight()) / 2;
    dc.DrawBitmap(bitmap_, x, y, true);
  }
}

void BitmapToggleButton::OnMouseLeftUp(wxMouseEvent& evt) {
  if (GetClientRect().Contains(evt.GetPosition())) {
    PostEvent();
  }

  if (HasCapture()) {
    ReleaseMouse();
  }

  toggle_ = !toggle_;

  pressed_ = false;
  Refresh();

  evt.Skip();
}

}  // namespace ui
}  // namespace jil
