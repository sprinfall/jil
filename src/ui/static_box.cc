#include "ui/static_box.h"
#include "wx/dcclient.h"
#include "wx/sizer.h"

namespace jil {
namespace ui {

 static const wxColour kBorderColour(213, 223, 229);

BEGIN_EVENT_TABLE(StaticBox, wxPanel)
EVT_PAINT(StaticBox::OnPaint)
END_EVENT_TABLE()

StaticBox::StaticBox(wxWindow* parent, const wxString& label)
    : wxPanel(parent, wxID_ANY)
    , border_colour_(kBorderColour) {
  SetLabel(label);

  SetBackgroundColour(parent->GetBackgroundColour());
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  padding_.x = GetCharHeight() * 0.7;
  padding_.y = padding_.x;
}

StaticBox::~StaticBox() {
}

void StaticBox::SetBodySizer(wxSizer* body_sizer) {
  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

  if (GetLabel().IsEmpty()) {
    vsizer->AddSpacer(padding_.y);
  } else {
    vsizer->AddSpacer(GetCharHeight() + padding_.y);
  }

  vsizer->Add(body_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT, padding_.x);

  vsizer->AddSpacer(padding_.y);

  SetSizer(vsizer);
}

void StaticBox::OnPaint(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  wxString label = GetLabel();

  dc.SetPen(wxPen(border_colour_));
  dc.SetBrush(*wxTRANSPARENT_BRUSH);

  wxRect border_rect = GetClientRect();

  if (!label.IsEmpty()) {
    int half_char_height = GetCharHeight() / 2;
    border_rect.y += half_char_height;
    border_rect.height -= half_char_height;
  }

  dc.DrawRoundedRectangle(border_rect, 3.0f);

  if (!label.IsEmpty()) {
    // Draw a rect to erase the border line under the label.
    int w = 0;
    int h = 0;
    dc.GetTextExtent(label, &w, &h);

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(GetBackgroundColour()));

    dc.DrawRectangle(padding_.x - 3, 0, w + 6, h);

    // Draw the label.
    dc.DrawText(label, padding_.x, 0);
  }
}

}  // namespace ui
}  // namespace jil
