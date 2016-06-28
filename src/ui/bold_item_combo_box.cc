#include "ui/bold_item_combo_box.h"

#include "wx/dcmemory.h"

namespace jil {
namespace ui {

BoldItemComboBox::BoldItemComboBox(wxWindow* parent, wxWindowID id)
  : wxOwnerDrawnComboBox(parent, id) {
  wxBitmap bmp(1, 1);
  measure_dc_ = new wxMemoryDC(bmp);
  measure_dc_->SetFont(GetFont().MakeBold());

  padding_.Set(3, 2);

  int ch = measure_dc_->GetCharHeight();
  row_height_ = ch + padding_.y + padding_.y;
}

BoldItemComboBox::~BoldItemComboBox() {
  wxDELETE(measure_dc_);
}

void BoldItemComboBox::OnDrawItem(wxDC& dc,
                                  const wxRect& rect,
                                  int item,
                                  int flags) const {
  if (item == wxNOT_FOUND) {
    return;
  }

  if (!(flags & wxODCB_PAINTING_CONTROL)) {
    if (bolds_[item]) {
      SetBoldFont(dc, true);
    }

    dc.DrawText(GetString(item), rect.x + padding_.x, rect.y + padding_.y);

    if (bolds_[item]) {
      SetBoldFont(dc, false);
    }
  }
}

wxCoord BoldItemComboBox::OnMeasureItem(size_t item) const {
  return row_height_;
}

wxCoord BoldItemComboBox::OnMeasureItemWidth(size_t item) const {
  int w = 0;
  measure_dc_->GetTextExtent(GetString(item), &w, NULL);
  return w + padding_.x + padding_.x;
}

void BoldItemComboBox::Append(const wxString& item, bool bold) {
  wxOwnerDrawnComboBox::Append(item);
  bolds_.push_back(bold);
}

void BoldItemComboBox::SetBoldFont(wxDC& dc, bool bold) const {
  wxFont font = dc.GetFont();
  font.SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
  dc.SetFont(font);
}

}  // namespace ui
}  // namespace jil
