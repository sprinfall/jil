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

class wxSizer;
#ifndef __WXMAC__
class wxMemoryDC;
#endif  // __WXMAC__
class wxGraphicsContext;

namespace jil {

// Page interface of book ctrl.
class BookPage {
 public:
  enum Flag {
    kModified = 1,
  };

  virtual ~BookPage() {
  }

  // Prefix "Page_" is added to each function to avoid name conflict.

  // Page as window.
  virtual wxWindow* Page_Window() = 0;

  // Activate/deactivate this page.
  virtual void Page_Activate(bool active) = 0;

  // Close this page (also destroy the window).
  virtual bool Page_Close() = 0;

  // Page label displayed in tab.
  virtual wxString Page_Label() const = 0;

  // Page description displayed, e.g., in tab tooltip.
  virtual wxString Page_Description() const {
    return wxEmptyString;
  }

  // See enum Flag.
  virtual int Page_Flags() const {
    return 0;
  }
};

class BookTabArea;

class BookPageArea : public wxPanel {
 public:
  BookPageArea(wxWindow* parent, wxWindowID id)
      : wxPanel(parent, id) {
  }
};

class BookCtrl : public wxPanel {
  DECLARE_EVENT_TABLE()

 public:
  enum ColorId {
    BG = 0,  // The whole book background

    TAB_AREA_BG,  // The tab area background

    // Tab item colors:
    TAB_FG,
    ACTIVE_TAB_FG,
    TAB_BG,
    ACTIVE_TAB_BG,
    TAB_BORDER,
    ACTIVE_TAB_BORDER,

    COLOR_COUNT
  };

  enum FontId {
    TAB_FONT = 0,  // Tab text font
    FONT_COUNT
  };

 protected:
  class Tab {
   public:
    Tab(BookPage* page_, int best_size_, bool active_ = false)
        : page(page_)
        , best_size(best_size_)
        , size(best_size_)
        , active(active_) {
    }

    BookPage* page;

    // Tab size.
    int best_size;  // Size fitting the label
    int size;  // Display size

    bool active;
  };

  typedef std::list<Tab*> TabList;

 public:
  explicit BookCtrl(const editor::SharedTheme& theme);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~BookCtrl();

  // Overriddens of wxWindow.
  virtual bool HasFocus() const override;

  wxPanel* PageParent() { return page_area_; }

  // Set batch flag to avoid unecessary resizing tabs and refresh.
  // Example:
  //   notebook->StartBatch();
  //   notebook->AddPage(...);
  //   notebook->AddPage(...);
  //   ...
  //   notebook->EndBatch();
  void StartBatch();
  void EndBatch();

  // NOTE: Don't check if the page is already in the book or not.
  virtual bool AddPage(BookPage* page, bool active = false);

  bool RemovePage(const BookPage* page);

  bool RemoveActivePage();

  // The except page, if specified, won't be removed.
  bool RemoveAllPages(const BookPage* except_page = NULL);

  size_t PageCount() const { return tabs_.size(); }

  void ActivatePage(BookPage* page);
  BookPage* ActivePage() const;

  void SwitchToNextPage();
  void SwitchToPrevPage();

  void SwitchToNextStackPage();
  void SwitchToPrevStackPage();

  std::vector<BookPage*> Pages() const;

  // Return the page next to the given page.
  BookPage* NextPage(const BookPage* page) const;

  // Recalculate the size for each tab.
  // Don't refresh even the tab size changes if @refresh is false.
  void ResizeTabs(bool refresh = true);

  BookPage* rclicked_tab_page() const {
    return rclicked_tab_ == NULL ? NULL : rclicked_tab_->page;
  }

 protected:
  friend class BookTabArea;
  void OnTabSize(wxSizeEvent& evt);

  void OnTabPaint(wxAutoBufferedPaintDC& dc, wxPaintEvent& evt);
  void DrawBackground(wxGraphicsContext* gc);
  void DrawForeground(wxGraphicsContext* gc, wxDC& dc);
  // Draw background for one given tab.
  void DrawTabBackground(Tab* tab, wxGraphicsContext* gc, int x);

  void OnTabMouse(wxMouseEvent& evt);

  void OnTabMouseLeftDown(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftDown(wxMouseEvent& evt);

  void OnTabMouseLeftUp(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftUp(wxMouseEvent& evt);

  void OnTabMouseMiddleDown(wxMouseEvent& evt);
  void OnTabMouseMiddleUp(wxMouseEvent& evt);

  void OnTabMouseMotion(wxMouseEvent& evt);

  void OnTabMouseRightDown(wxMouseEvent& evt);
  virtual void HandleTabMouseRightDown(wxMouseEvent& evt);

  void OnTabMouseRightUp(wxMouseEvent& evt);
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt);

  void OnTabMouseLeftDClick(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftDClick(wxMouseEvent& evt);

  TabList::iterator TabByPos(int pos_x);
  Tab* GetTabByWindow(wxWindow* window, size_t* index = NULL);

  void ActivatePage(TabList::iterator it);
  bool RemovePage(TabList::iterator it);

  TabList::iterator ActiveTab();
  TabList::iterator TabByPage(const BookPage* page);
  TabList::const_iterator TabByPage(const BookPage* page) const;

  int CalcTabBestSize(const wxString& label) const;

  void RefreshTabArea();

  void PostEvent(wxEventType event_type);

 protected:
  editor::SharedTheme theme_;

  int tab_min_size_;
  int tab_default_size_;

  // Tab area free/available size.
  int free_size_;

  BookTabArea* tab_area_;
  BookPageArea* page_area_;
  wxSizer* page_sizer_;

  TabList tabs_;

  // The front tab is the current active tab.
  // A tab is moved to the front whenever it's activated.
  TabList tab_stack_;

  Tab* rclicked_tab_;

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
