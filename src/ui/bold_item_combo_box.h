#ifndef JIL_UI_BOLD_ITEM_COMBO_BOX_H_
#define JIL_UI_BOLD_ITEM_COMBO_BOX_H_
#pragma once

#include <vector>
#include "wx/odcombo.h"

namespace jil {
namespace ui {

// Owner drawn combo box for displaying items in bold.
class BoldItemComboBox : public wxOwnerDrawnComboBox {
public:
  BoldItemComboBox(wxWindow* parent, wxWindowID id);
  virtual ~BoldItemComboBox();

  virtual void OnDrawItem(wxDC& dc,
                          const wxRect& rect,
                          int item,
                          int flags) const override;

  virtual wxCoord OnMeasureItem(size_t item) const override;

  virtual wxCoord OnMeasureItemWidth(size_t item) const override;

  // NOTE: Please add items with this function.
  void Append(const wxString& item, bool bold);

private:
  void SetBoldFont(wxDC& dc, bool bold) const;

private:
  wxSize padding_;
  wxCoord row_height_;
  std::vector<bool> bolds_;
  wxDC* measure_dc_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BOLD_ITEM_COMBO_BOX_H_