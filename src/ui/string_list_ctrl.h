#ifndef UI_STRING_LIST_CTRL_H_
#define UI_STRING_LIST_CTRL_H_

// NOTE:
// StringListCtrl emits the same event class (wxListEvent) as wxListCtrl:
//   - Event types: wxEVT_LIST_ITEM_SELECTED, wxEVT_LIST_ITEM_DESELECTED
//   - Macros: EVT_LIST_ITEM_SELECTED, EVT_LIST_ITEM_DESELECTED

#include <vector>

#include "wx/scrolwin.h"

namespace ui {

class StringListCtrl : public wxScrolledWindow {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    COLOR_BG = 0,
    COLOR_BG_HL,
    COLOR_FG,
    COLOR_FG_HL,
    COLOR_BORDER,
    COLOR_COUNT,
  };

public:
  StringListCtrl();
  virtual ~StringListCtrl();

  bool AcceptsFocus() const override {
    return true;
  }

  bool Create(wxWindow* parent,
              wxWindowID id,
              const wxSize& size = wxDefaultSize,
              long style = wxScrolledWindowStyle);

  const wxColour& GetColor(ColorId id) const {
    return colors_[id];
  }

  void SetColor(ColorId id, const wxColour& color) {
    colors_[id] = color;
  }

  void AppendString(const wxString& string);

  void UpdateSizes();

  int GetCount() const {
    return static_cast<int>(strings_.size());
  }

  int selected_index() const {
    return selected_index_;
  }

protected:
  void InitColors();

  wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnFocusChange(wxFocusEvent& evt);

  void DrawSelectedRow(wxDC& dc, const wxRect& row_rect, const wxString& label);

  void PostEvent(int index, bool select);

private:
  wxColour colors_[COLOR_COUNT];

  std::vector<wxString> strings_;

  // 0-based index of the current selection.
  // No selection if it's wxNOT_FOUND (-1).
  int selected_index_;

  wxSize raw_padding_;
  wxSize row_size_;
};

}  // namespace ui

#endif  // UI_STRING_LIST_CTRL_H_
