#ifndef JIL_POPUP_MENU_H_
#define JIL_POPUP_MENU_H_

#include <vector>

#include "wx/popupwin.h"

#include "editor/theme.h"

#include "jil/defs.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

// NOTE:
// - Don't have to handle key down event of ESC, wxPopupWindow handles it.

class PopupMenu : public wxPopupTransientWindow, public WithPopupTheme {
  DECLARE_EVENT_TABLE()

public:
  explicit PopupMenu(const editor::SharedTheme& theme);

  virtual ~PopupMenu();

  bool Create(wxWindow* parent, wxWindowID id);

  // Override to destroy the window.
  virtual void Dismiss() override;

  // Override to adjust the client size before show.
  virtual void Popup(wxWindow* focus = NULL) override;

  void SetTabs(const std::vector<wxString>& tabs);

  void set_column_width(int column_width) {
    column_width_ = column_width;
  }

  void set_max_rows(int max_rows) {
    max_rows_ = max_rows;
  }

  size_t select_index() const {
    return select_index_;
  }

  void set_owner_rect(const wxRect& rect) {
    owner_rect_ = rect;
  }

  void set_display_client_rect(const wxRect& rect) {
    display_client_rect_ = rect;
  }

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseMotion(wxMouseEvent& evt);

private:
  void AdjustSizeAndPosition();

  void UpdateItemRects();

  void CalcMaxRows(int total_height);

  // Calculate the actual number of rows and columns.
  void CalcRowsCols();

  size_t GetIndexByPos(const wxPoint& pos) const;

private:
  editor::SharedTheme theme_;

  std::vector<wxString> items_;

  wxSize margin_;
  wxSize item_padding_;
  int col_gap_;

  int row_height_;

  size_t select_index_;

  int column_width_;

  // The max number of items displayed in one column.
  int max_rows_;

  int rows_;
  int cols_;

  std::vector<wxRect> item_rects_;

  wxRect owner_rect_;
  wxRect display_client_rect_;
};

////////////////////////////////////////////////////////////////////////////////
// Event for popup menu.

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(kEvtPopupMenuSelect, 0)
END_DECLARE_EVENT_TYPES()

class PopupMenuEvent : public wxEvent {
  DECLARE_DYNAMIC_CLASS_NO_ASSIGN(PopupMenuEvent)

public:
  PopupMenuEvent(wxEventType type = wxEVT_NULL);
  PopupMenuEvent(const PopupMenuEvent& rhs);

  virtual wxEvent* Clone() const override {
    return new PopupMenuEvent(*this);
  }

  size_t index() const {
    return index_;
  }

  void set_index(size_t index) {
    index_ = index;
  }

  const wxString& label() const {
    return label_;
  }

  void set_label(const wxString& label) {
    label_ = label;
  }

private:
  size_t index_;
  wxString label_;
};

typedef void (wxEvtHandler::*PopupMenuEventFunction)(PopupMenuEvent&);

}  // namespace jil

#define PopupMenuEventHandler(func)\
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(::jil::PopupMenuEventFunction, &func)

#define EVT_POPUP_MENU_SELECT(func)\
  wx__DECLARE_EVT0(::jil::kEvtPopupMenuSelect, PopupMenuEventHandler(func))

#endif  // JIL_POPUP_MENU_H_
