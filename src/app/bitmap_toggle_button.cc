#include "app/bitmap_toggle_button.h"
#include "wx/dcclient.h"

namespace jil {

enum {
  kPaddingX = 5,
  kPaddingY = 3,
};

BEGIN_EVENT_TABLE(BitmapToggleButton, wxControl)
EVT_PAINT     (BitmapToggleButton::OnPaint)
EVT_LEFT_DOWN (BitmapToggleButton::OnLeftDown)
EVT_LEFT_UP   (BitmapToggleButton::OnLeftUp)
END_EVENT_TABLE()

BitmapToggleButton::BitmapToggleButton(wxWindow* parent, wxWindowID id)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , accept_focus_(false), toggle_(false) {
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

BitmapToggleButton::~BitmapToggleButton() {
}

bool BitmapToggleButton::Enable(bool enable) {
  if (wxControl::Enable(enable)) {
    Refresh();
    return true;
  }
  return false;
}

wxSize BitmapToggleButton::DoGetBestSize() const {
  if (!bitmap_.IsOk()) {
    return wxDefaultSize;
  }
  return wxSize(bitmap_.GetWidth(), bitmap_.GetHeight());
}

void BitmapToggleButton::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  wxBitmap bitmap;
  if (IsEnabled()) {
    bitmap = toggle_ ? toggle_bitmap_ : bitmap_;
  } else {
    bitmap = toggle_ ? disabled_toggle_bitmap_ : disabled_bitmap_;
  }

  if (bitmap.IsOk()) {
    dc.DrawBitmap(bitmap, 0, 0, true);
  }
}

void BitmapToggleButton::OnLeftDown(wxMouseEvent& evt) {
  if (!HasCapture()) {
    CaptureMouse();
  }
}

void BitmapToggleButton::OnLeftUp(wxMouseEvent& evt) {
  if (HasCapture()) {
    ReleaseMouse();
  }

  toggle_ = !toggle_;
  Refresh();

  wxCommandEvent toggle_evt(wxEVT_TOGGLEBUTTON, GetId());
  toggle_evt.SetEventObject(this);
  // Note: SetInt() affects IsChecked().
  toggle_evt.SetInt(toggle_ ? 1 : 0);
  GetParent()->GetEventHandler()->AddPendingEvent(toggle_evt);
}

}  // namespace jil
