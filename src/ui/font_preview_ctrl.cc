#include "ui/font_preview_ctrl.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

static const wxSize kPadding(5, 1);

BEGIN_EVENT_TABLE(FontPreviewCtrl, wxControl)
EVT_PAINT       (FontPreviewCtrl::OnPaint)
EVT_LEFT_DOWN   (FontPreviewCtrl::OnMouseLeftDown)
EVT_LEFT_UP     (FontPreviewCtrl::OnMouseLeftUp)
END_EVENT_TABLE()

FontPreviewCtrl::FontPreviewCtrl(wxWindow* parent, wxWindowID id, const wxFont& font)
    : wxControl(parent, id) {
  SetFont(font);

  SetBackgroundColour(parent->GetBackgroundColour());
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

FontPreviewCtrl::~FontPreviewCtrl() {
}

wxSize FontPreviewCtrl::DoGetBestSize() const {
  wxSize size = GetTextExtent(GetFontLabel());
  size.IncBy(GetWindowBorderSize());
  size.IncBy(kPadding);
  size.IncBy(kPadding);
  return size;
}

void FontPreviewCtrl::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  dc.SetTextForeground(GetForegroundColour());

  wxFont font = GetFont();
  if (font.IsOk()) {
    dc.SetFont(font);
    dc.DrawText(GetFontLabel(), kPadding.x, kPadding.y);
  }
}

void FontPreviewCtrl::OnMouseLeftDown(wxMouseEvent& evt) {
  if (!HasCapture()) {
    CaptureMouse();
  }
  evt.Skip();
}

void FontPreviewCtrl::OnMouseLeftUp(wxMouseEvent& evt) {
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

wxString FontPreviewCtrl::GetFontLabel() const {
  wxFont font = GetFont();

  wxString name = font.GetFaceName();
  wxString size = wxString::Format(wxT("%d"), font.GetPointSize());

  // Vim: set guifont=Consolas:h11:cDEFAULT
  return (name + wxT(" : ") + size); 
}

}  // namespace ui
}  // namespace jil
