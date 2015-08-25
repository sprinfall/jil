#ifndef JIL_NAVIGATION_DIALOG_H_
#define JIL_NAVIGATION_DIALOG_H_
#pragma once

#include <vector>
#include "wx/dialog.h"
#include "editor/theme.h"

namespace jil {

class TextPage;

class NavigationDialog : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BG = 0,
    FG,
    SELECT_BG,
    SELECT_BORDER,
    SELECT_FG,
    COLOR_COUNT,
  };

public:
  NavigationDialog(wxWindow* parent,
                   wxWindowID id,
                   const editor::SharedTheme& theme);
  virtual ~NavigationDialog();

  // Override to adjust the client size before show.
  virtual int ShowModal() override;

  // \param forward Switch to next or previous stack page initially.
  void SetTextPages(const std::vector<TextPage*>& text_pages, bool forward);

  void set_column_width(int column_width) {
    column_width_ = column_width;
  }

  void set_max_rows(int max_rows) {
    max_rows_ = max_rows;
  }

  size_t select_index() const {
    return select_index_;
  }

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnClose(wxCloseEvent& evt);
  void OnActivate(wxActivateEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseMotion(wxMouseEvent& evt);
  void OnKeyDown(wxKeyEvent& evt);
  void OnKeyUp(wxKeyEvent& evt);
  void OnNavigationKey(wxNavigationKeyEvent& evt);

private:
  void AdjustSize();

  size_t GetIndexByPos(const wxPoint& pos) const;

private:
  editor::SharedTheme theme_;

  // Active text pages.
  std::vector<TextPage*> text_pages_;

  size_t select_index_;

  int column_width_;  // In pixel

  // The max number of items displayed in one column.
  int max_rows_;

  wxFont title_font_;

  wxRect title_rect_;
  std::vector<wxRect> text_rects_;
  wxRect path_rect_;
};

}  // namespace jil

#endif  // JIL_NAVIGATION_DIALOG_H_
