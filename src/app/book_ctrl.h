#ifndef JIL_BOOK_CTRL_H_
#define JIL_BOOK_CTRL_H_
#pragma once

// A replacement of wxNotebook.

#include <list>
#include <vector>

#include "wx/panel.h"
#include "wx/dcbuffer.h"

#include "ui/button_style.h"

#include "editor/theme.h"

#include "app/compile_config.h"

class wxMenu;
class wxSizer;

#ifndef __WXMAC__
class wxMemoryDC;
#endif  // __WXMAC__

namespace jil {

namespace ui {
class BitmapButton;
}

#if defined(__WXMSW__)
namespace editor {
class TipHandler;
}  // namespace editor
#endif  // __WXMSW__

class BookCtrl;
class BookPage;
class PopupMenuEvent;

////////////////////////////////////////////////////////////////////////////////

class TabPanel : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  TabPanel(BookCtrl* book_ctrl, wxWindowID id);
  virtual ~TabPanel();

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
  DECLARE_EVENT_TABLE()

public:
  enum ThemeId {
    THEME_BUTTON = 0,

    THEMES,
  };

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
    IMAGE_TAB_MORE,

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

  void set_popup_theme(const editor::SharedTheme& popup_theme) {
    popup_theme_ = popup_theme;
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

  // Remove the given page.
  // Return true if the page is removed.
  bool RemovePage(const BookPage* page);

  // Remove the current active page.
  // Return true if the page is removed.
  bool RemoveActivePage();

  // Remove all pages.
  // \param from_destroy It's going to be destroyed, don't refresh or post event.
  // \param except_page If specified, this page won't be removed.
  void RemoveAllPages(bool from_destroy, const BookPage* except_page = NULL);

  size_t PageCount() const {
    return tabs_.size();
  }

  bool IsEmpty() const {
    return PageCount() == 0;
  }

  // Activate the given page.
  // \param ensure_visible Make sure the tab of this page will be visible.
  void ActivatePage(BookPage* page, bool ensure_visible);

  // Return the current active page.
  BookPage* ActivePage() const;

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

  void CreateTabPanel();

  // Update values determined by tab font.
  void UpdateTabFontDetermined();

  //----------------------------------------------------------------------------
  // Tab area event handlers.

  friend class TabPanel;

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

  void OnTabMoreButtonClick(wxCommandEvent& evt);
  void OnTabMoreButtonUpdateUI(wxUpdateUIEvent& evt);

  void OnMoreTabsMenuSelect(PopupMenuEvent& evt);

  // Move the given tab so that it becomes visible.
  // \param check Check if the tab is really invisible.
  // \param refresh Refresh the tab panel if necessary.
  // \return The new iterator of the tab.
  TabIter MakeTabVisible(TabIter it, bool check = false, bool refresh = true);

  TabIter MakeActiveTabVisible(bool refresh);

  //----------------------------------------------------------------------------

  // Get tab iterator by index.
  TabIter TabIterByIndex(size_t index);

  // Get tab iterator by x position.
  // \param tab_rect Optional output of the tab rect.
  TabIter TabIterByPos(int pos_x, wxRect* tab_rect = NULL);

  // Get tab by x position.
  // \param tab_rect Optional output of the tab rect.
  Tab* TabByPos(int pos_x, wxRect* tab_rect = NULL);

  BookPage* PageByPos(int pos_x);

  // \param ensure_visible Make sure the active tab will be visible.
  void ActivatePage(TabIter it, bool ensure_visible);

  bool RemovePage(TabIter it);

  virtual void DoActivateTab(Tab* tab, bool active) = 0;
  virtual void DoRemoveTab(Tab* tab) = 0;
  virtual void DoRemoveAll(Tab* tab) = 0;

  void ActivatePageByPos(int pos_x);

  Tab* ActiveTab() const;

  TabIter ActiveTabIter();

  TabIter VisibleEndTabIter();

  TabIter TabIterByPage(const BookPage* page);
  TabConstIter TabIterByPage(const BookPage* page) const;

  int CalcTabBestSize(const wxString& label) const;

  wxSize CalcTabPanelBestSize() const;

  wxRect GetTabRect(int x, int width, const wxRect& tab_area_rect);
  wxRect GetTabCloseIconRect(const wxRect& tab_rect);

  void PostEvent(wxEventType event_type);

protected:
  editor::SharedTheme theme_;

  // Theme for the popup menu displaying hidden tabs.
  editor::SharedTheme popup_theme_;

  int char_width_;
  int ellipsis_width_;  // Size of "...".

  int tab_margin_top_;
  wxSize tab_padding_;

  // The space between two elements inside a tab.
  int tab_space_x_;

  // The space before/after the first/last tab.
  int tab_panel_padding_x_;

  // Tab panel displays the tabs.
  // If there is not enough space to display all tabs, it just displays
  // first visible_tabs_count_ number of tabs.
  TabPanel* tab_panel_;

  ui::BitmapButton* tab_more_button_;

  wxPanel* page_panel_;

  TabList tabs_;

  // The front tab is the current active tab.
  // A tab is moved to the front when it's activated.
  TabList stack_tabs_;

  // Number of visible tabs.
  size_t visible_tabs_count_;

  // The tab on which the mouse is hovering.
  Tab* hover_tab_;
  // If the mouse is hovering on the close icon of the tab.
  bool hover_on_close_icon_;

  // Batch mode.
  bool batch_;
};

////////////////////////////////////////////////////////////////////////////////
// Events posted by BookCtrl.

BEGIN_DECLARE_EVENT_TYPES()
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
