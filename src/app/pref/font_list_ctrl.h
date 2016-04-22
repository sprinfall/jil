#ifndef JIL_PREF_FONT_LIST_CTRL_H_
#define JIL_PREF_FONT_LIST_CTRL_H_
#pragma once

// wxListEvent
// EVT_LIST_ITEM_SELECTED(id, func)
// wxEVT_LIST_ITEM_SELECTED
// EVT_LIST_ITEM_DESELECTED(id, func)
// wxEVT_LIST_ITEM_DESELECTED

#include <vector>
#include "wx/scrolwin.h"

namespace jil {
namespace pref {

class FontListCtrl : public wxScrolledWindow {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    COLOR_BG = 0,
    COLOR_BG_HL,
    COLOR_BG_HL_NOFOCUS,
    COLOR_FG,
    COLOR_FG_HL,
    COLOR_BORDER,
    COLOR_COUNT
  };

public:
  FontListCtrl();
  virtual ~FontListCtrl();

  virtual bool AcceptsFocus() const override {
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

  void AddFont(const wxFont& font, const wxString& label);

  void SetFont(int index, const wxFont& font);

  void UpdateSizes();

  int GetCount() const {
    return static_cast<int>(rows_.size());
  }

  int selected_index() const {
    return selected_index_;
  }

protected:
  void InitColors();

  void OnSize(wxSizeEvent& evt);

  void OnPaint(wxPaintEvent& evt);

  void OnMouseLeftDown(wxMouseEvent& evt);

  void OnSetFocus(wxFocusEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);

  void PostEvent(int index, bool select);

private:
  wxColour colors_[COLOR_COUNT];

  struct Row {
    wxFont font;
    wxString label;
    int y;
    int h;
  };

  std::vector<Row> rows_;

  // 0-based index of the current selection.
  // No selection if it's wxNOT_FOUND (-1).
  int selected_index_;

  wxSize padding_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_FONT_LIST_CTRL_H_
