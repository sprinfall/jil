#include "ui/font_preview_ctrl.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

 static const wxSize kPadding(5, 2);

BEGIN_EVENT_TABLE(FontPreviewCtrl, wxControl)
EVT_PAINT(FontPreviewCtrl::OnPaint)
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
  wxSize size = GetTextExtent(GetFont().GetFaceName());
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
    dc.DrawText(font.GetFaceName(), kPadding.x, kPadding.y);
  }
}

}  // namespace ui
}  // namespace jil
