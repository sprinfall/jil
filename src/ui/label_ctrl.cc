#include "ui/label_ctrl.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

static const wxSize kPadding(5, 1);

BEGIN_EVENT_TABLE(LabelCtrl, wxControl)
EVT_PAINT(LabelCtrl::OnPaint)
EVT_LEFT_DOWN(LabelCtrl::OnMouseLeftDown)
EVT_LEFT_UP(LabelCtrl::OnMouseLeftUp)
EVT_MOUSE_CAPTURE_LOST(LabelCtrl::OnMouseCaptureLost)
END_EVENT_TABLE()

LabelCtrl::LabelCtrl(wxWindow* parent, wxWindowID id, const wxString& label)
    : wxControl(parent, id) {
  SetLabel(label);
  SetBackgroundColour(parent->GetBackgroundColour());
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

LabelCtrl::~LabelCtrl() {
}

wxSize LabelCtrl::DoGetBestSize() const {
  wxSize size = GetTextExtent(GetLabel());
  size.IncBy(GetWindowBorderSize());
  size.IncBy(kPadding);
  size.IncBy(kPadding);
  return size;
}

void LabelCtrl::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  dc.SetTextForeground(GetForegroundColour());
  dc.SetFont(GetFont());

  dc.DrawText(GetLabel(), kPadding.x, kPadding.y);
}

void LabelCtrl::OnMouseLeftDown(wxMouseEvent& evt) {
  if (!HasCapture()) {
    CaptureMouse();
  }
  evt.Skip();
}

void LabelCtrl::OnMouseLeftUp(wxMouseEvent& evt) {
  if (!HasCapture()) {
    return;
  }
  ReleaseMouse();

  if (!GetClientRect().Contains(evt.GetPosition())) {
    return;
  }

  wxCommandEvent button_evt(wxEVT_BUTTON, GetId());
  button_evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(button_evt);
}

void LabelCtrl::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Do nothing.
}

}  // namespace ui
}  // namespace jil
