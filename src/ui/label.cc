#include "ui/label.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

BEGIN_EVENT_TABLE(Label, wxControl)
EVT_PAINT     (Label::OnPaint)
EVT_LEFT_DOWN (Label::OnLeftDown)
EVT_LEFT_UP   (Label::OnLeftUp)
END_EVENT_TABLE();

Label::Label(wxWindow* parent, wxWindowID id, const wxString& label)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , accept_focus_(false) {
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  SetBackgroundColour(parent->GetBackgroundColour());

  SetLayoutDirection(wxLayout_LeftToRight);

  SetLabel(label);

  DeterminePadding();
}

Label::~Label() {
}

// Determine padding by char width.
void Label::DeterminePadding() {
  int cw = GetCharWidth();
  padding_.Set(cw, cw / 2);
}

wxSize Label::DoGetBestSize() const {
  wxSize best_size;
  GetTextExtent(GetLabel(), &best_size.x, &best_size.y);
  best_size.IncBy(padding_.x * 2, padding_.y * 2);
  best_size.IncBy(1, 1);  // Border
  return best_size;
}

void Label::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  const wxRect client_rect = GetClientRect();

  if (border_color_.IsOk()) {
    dc.SetPen(wxPen(border_color_, 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(client_rect);
  }

  dc.SetTextForeground(GetForegroundColour());
  dc.SetFont(GetFont());

  wxRect label_rect = client_rect.Deflate(padding_.x, 0);
  dc.DrawLabel(GetLabel(), label_rect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
}

void Label::OnLeftDown(wxMouseEvent& evt) {
  if (!HasCapture()) {
    CaptureMouse();
  }
}

void Label::OnLeftUp(wxMouseEvent& evt) {
  if (HasCapture()) {
    ReleaseMouse();
  }

  wxCommandEvent click_evt(wxEVT_BUTTON, GetId());
  click_evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(click_evt);
}

}  // namespace ui
}  // namespace jil
