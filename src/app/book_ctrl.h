#ifndef JIL_BOOK_CTRL_H_
#define JIL_BOOK_CTRL_H_
#pragma once

// A replacement of wxNotebook.

#include <list>
#include <vector>
#include "wx/panel.h"
#include "wx/dcbuffer.h"
#include "editor/theme.h"
#include "app/compile_config.h"

class wxMenu;
class wxSizer;

#ifndef __WXMAC__
class wxMemoryDC;
#endif  // __WXMAC__

namespace jil {

#if defined(__WXMSW__)
namespace editor {
class TipHandler;
}  // namespace editor
#endif  // __WXMSW__

class BookCtrl;
class BookPage;

////////////////////////////////////////////////////////////////////////////////

class BookTabArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  BookTabArea(BookCtrl* book_ctrl, wxWindowID id);

  virtual ~BookTabArea();

  // Page header has no focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

#if defined(__WXMSW__)
  void SetToolTipEx(const wxString& tooltip);
#endif

protected:
  virtual wxSize DoGetBestSize() const override;

  void OnSize(wxSizeEvent& evt);
  void OnPaint(wxPaintEvent& evt);
  void OnMouseEvents(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
  void OnLeaveWindow(wxMouseEvent& evt);

private:
  BookCtrl* book_ctrl_;

#if defined(__WXMSW__)
  editor::TipHandler* tip_handler_;
#endif
};

////////////////////////////////////////////////////////////////////////////////

class BookCtrl : public wxPanel {
public:
  enum ColorId {
    COLOR_BG = 0,  // The whole book background

    COLOR_TAB_AREA_BG,  // The tab area background

    // Tab item colors:
    COLOR_TAB_FG,
    COLOR_TAB_BG,
    COLOR_TAB_BORDER,

    COLOR_ACTIVE_TAB_FG,
    COLOR_ACTIVE_TAB_BG,
    COLOR_ACTIVE_TAB_BORDER,

    COLOR_TAB_HOVER_BG,
    COLOR_ACTIVE_TAB_HOVER_BG,

    COLORS,
  };

  enum ImageId {
    IMAGE_TAB_CLOSE = 0,

    IMAGES,
  };

protected:
  class Tab {
  public:
    Tab(BookPage* _page, int _best_size, bool _active)
        : page(_page)
        , best_size(_best_size)
        , size(_best_size)
        , active(_active) {
    }

    BookPage* page;

    // Tab size.
    int best_size;  // Size fitting the label
    int size;  // Display size

    bool active;
  };

  typedef std::list<Tab*> TabList;
  typedef TabList::iterator TabIter;
  typedef TabList::const_iterator TabConstIter;

public:
  BookCtrl();
  virtual ~BookCtrl();

  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  bool Create(wxWindow* parent, wxWindowID id);

  virtual bool HasFocus() const override;
  virtual void SetFocus() override;

  // NOTE: Call after Create().
  void SetTabFont(const wxFont& tab_font);

  virtual void ReapplyTheme();

  // Set batch flag to avoid unnecessary resizing tabs and refresh.
  // Example:
  //   notebook->StartBatch();
  //   notebook->AddPage(...);
  //   notebook->AddPage(...);
  //   ...
  //   notebook->EndBatch();
  void StartBatch();
  void EndBatch();

  // NOTE: Don't check if the page is already in the book or not.
  virtual void AddPage(BookPage* page, bool active);

  bool RemovePage(const BookPage* page);

  bool RemoveActivePage();

  // \param from_destroy If from destroy, don't refresh or post event.
  // \param except_page If specified, this page won't be removed.
  void RemoveAllPages(bool from_destroy, const BookPage* except_page = NULL);

  size_t PageCount() const {
    return tabs_.size();
  }

  bool IsEmpty() const {
    return PageCount() == 0;
  }

  void ActivatePage(BookPage* page);

  BookPage* ActivePage() const;

  void SwitchToNextPage();
  void SwitchToPrevPage();

  int GetStackIndex(BookPage* page) const;

  void MovePageToStackFront(BookPage* page);

  std::vector<BookPage*> Pages() const;
  std::vector<BookPage*> StackPages() const;

  // Return the page next to the given page.
  BookPage* NextPage(const BookPage* page) const;

  // Recalculate the size for each tab.
  void ResizeTabs();

  // Recalculate the active tab's best size, then call ResizeTabs().
  void ResizeActiveTab();

protected:
  void Init();

  // Update values determined by tab font.
  void UpdateTabFontDetermined();

  //----------------------------------------------------------------------------
  // Tab area event handlers.

  friend class BookTabArea;

  void OnTabSize(wxSizeEvent& evt);

  void OnTabPaint(wxDC& dc, wxPaintEvent& evt);

  void OnTabMouse(wxMouseEvent& evt);
  void OnTabMouseLeftDown(wxMouseEvent& evt);
  void OnTabMouseLeftUp(wxMouseEvent& evt);
  void OnTabMouseMiddleDown(wxMouseEvent& evt);
  void OnTabMouseMiddleUp(wxMouseEvent& evt);
  void OnTabMouseMotion(wxMouseEvent& evt);
  void OnTabMouseRightDown(wxMouseEvent& evt);
  void OnTabMouseRightUp(wxMouseEvent& evt);
  void OnTabMouseLeftDClick(wxMouseEvent& evt);
  void OnTabLeaveWindow(wxMouseEvent& evt);

  virtual void HandleTabMouseLeftDown(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftUp(wxMouseEvent& evt);
  virtual void HandleTabMouseMiddleUp(wxMouseEvent& evt);
  virtual void HandleTabMouseRightDown(wxMouseEvent& evt);
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt) {}
  virtual void HandleTabMouseLeftDClick(wxMouseEvent& evt) {}

  void SetTabTooltip(const wxString& tooltip);

  //----------------------------------------------------------------------------

  // Get tab iterator by x position.
  // \param tab_rect Optional output of the tab rect.
  TabIter TabIterByPos(int pos_x, wxRect* tab_rect = NULL);

  // Get tab by x position.
  // \param tab_rect Optional output of the tab rect.
  Tab* TabByPos(int pos_x, wxRect* tab_rect = NULL);

  BookPage* PageByPos(int pos_x);

  void ActivatePage(TabIter it);
  bool RemovePage(TabIter it);

  virtual void DoActivateTab(Tab* tab, bool active) = 0;
  virtual void DoRemoveTab(Tab* tab) = 0;
  virtual void DoRemoveAll(Tab* tab) = 0;

  void ActivatePageByPos(int pos_x);

  Tab* ActiveTab() const;

  TabIter ActiveTabIter();
  TabIter TabIterByPage(const BookPage* page);
  TabConstIter TabIterByPage(const BookPage* page) const;

  int CalcTabBestSize(const wxString& label) const;

  wxSize CalcTabAreaBestSize() const;

  wxRect GetTabRect(int x, int width, const wxRect& tab_area_rect);
  wxRect GetTabCloseIconRect(const wxRect& tab_rect);

  void PostEvent(wxEventType event_type);

protected:
  editor::SharedTheme theme_;

  int char_width_;
  int ellipsis_width_;  // Size of "...".

  int tab_margin_top_;
  wxSize tab_padding_;

  // The space between two elements inside a tab.
  int tab_space_x_;

  // The space before/after the first/last tab.
  int tab_area_padding_x_;

  int tab_min_size_;
  int tab_default_size_;

  // Tab area free/available size.
  int free_size_;

  BookTabArea* tab_area_;
  wxPanel* page_area_;

  TabList tabs_;

  // The front tab is the current active tab.
  // A tab is moved to the front when it's activated.
  TabList stack_tabs_;

  Tab* hover_tab_;
  bool hover_on_close_icon_;

  bool batch_;
  bool need_resize_tabs_;
};

////////////////////////////////////////////////////////////////////////////////
// Events

BEGIN_DECLARE_EVENT_TYPES()
// Events posted by book ctrl:
DECLARE_EVENT_TYPE(kEvtBookPageChange, 0)  // Page(s) added or removed.
DECLARE_EVENT_TYPE(kEvtBookPageSwitch, 0)  // Page activated.
END_DECLARE_EVENT_TYPES()

}  // namespace jil

#define EVT_BOOK_PAGE_CHANGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::kEvtBookPageChange, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#define EVT_BOOK_PAGE_SWITCH(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::kEvtBookPageSwitch, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_BOOK_CTRL_H_
