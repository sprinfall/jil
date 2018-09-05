#ifndef JIL_NAVIGATION_DIALOG_H_
#define JIL_NAVIGATION_DIALOG_H_

#include <vector>

#include "wx/dialog.h"

#include "editor/theme.h"

#include "jil/defs.h"

namespace jil {

class TextPage;

class NavigationDialog : public wxDialog, public WithPopupTheme {
  DECLARE_EVENT_TABLE()

public:
  explicit NavigationDialog(const editor::SharedTheme& theme);

  virtual ~NavigationDialog();

  bool Create(wxWindow* parent, wxWindowID id);

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

  std::size_t select_index() const {
    return select_index_;
  }

protected:
  virtual bool DoNavigateIn(int flags) override;

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

  std::size_t GetIndexByPos(const wxPoint& pos) const;

private:
  editor::SharedTheme theme_;

  wxSize margin_;
  int col_gap_;

  // Active text pages.
  std::vector<TextPage*> text_pages_;

  std::size_t select_index_;

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
