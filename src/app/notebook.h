#ifndef JIL_NOTEBOOK_H_
#define JIL_NOTEBOOK_H_
#pragma once

#include "wx/panel.h"
#include <list>
#include <vector>
#include "wx/dcbuffer.h"
#include "editor/theme.h"

class wxSizer;
#ifndef __WXMAC__
class wxMemoryDC;
#endif // __WXMAC__
class wxGraphicsContext;

namespace jil {\

class TextPage;

namespace notebook {\

class TabArea;

////////////////////////////////////////////////////////////////////////////////

class PageArea : public wxPanel {
public:
  PageArea(wxWindow* parent, wxWindowID id)
      : wxPanel(parent, id) {
  }
};

} // namespace notebook

class Notebook : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BG = 0, // The whole notebook background

    TAB_AREA_BG, // The tab area background

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
    TAB_FONT = 0, // Tab text font
    FONT_COUNT
  };

protected:
  class Tab {
  public:
    Tab(TextPage* page_, int best_size_, bool active_ = false)
        : page(page_)
        , best_size(best_size_)
        , size(best_size_)
        , active(active_) {
    }

    TextPage* page;

    // Tab size.
    int best_size; // Size fitting the label
    int size; // Display size

    bool active;
  };

  typedef std::list<Tab*> TabList;

public:
  Notebook(const editor::SharedTheme& theme);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~Notebook();

  wxPanel* GetPageParent() { return page_area_; }

  // Set batch flag to avoid unecessary resizing tabs and refresh.
  // Example:
  //   notebook->StartBatch();
  //   notebook->AddPage(...);
  //   notebook->AddPage(...);
  //   ...
  //   notebook->EndBatch();
  void StartBatch();
  void EndBatch();

  virtual bool AddPage(TextPage* page, bool active = false);
  bool RemovePage(const TextPage* page);
  bool RemoveActivePage();
  // The except page, if specified, won't be removed.
  bool RemoveAllPages(const TextPage* except_page = NULL);

  size_t PageCount() const { return tabs_.size(); }

  void ActivatePage(TextPage* page);
  TextPage* GetActivePage() const;

  void SwitchPage();

  std::vector<TextPage*> GetPages() const;

  // Recalculate the size for each tab.
  // Don't refresh even the tab size changes if @refresh is false.
  void ResizeTabs(bool refresh = true);

  const TextPage* rclicked_tab_page() const {
    return rclicked_tab_ == NULL ? NULL : rclicked_tab_->page;
  }

protected:
  // Tab area event handlers.
  friend class notebook::TabArea;
  void OnTabSize(wxSizeEvent& evt);

  void OnTabPaint(wxAutoBufferedPaintDC& dc, wxPaintEvent& evt);
  void DrawBackground(wxGraphicsContext* gc);
  void DrawForeground(wxGraphicsContext* gc, wxDC& dc);
  // Draw background for one given tab.
  void DrawTabBackground(Tab* tab, wxGraphicsContext* gc, int x);

  void OnTabMouse(wxMouseEvent& evt);
  void HandleTabMouseLeftUp(wxMouseEvent& evt);
  void HandleTabMouseMiddleDown(wxMouseEvent& evt);
  void HandleTabMouseMiddleUp(wxMouseEvent& evt);
  void HandleTabMouseMotion(wxMouseEvent& evt);
  void HandleTabMouseRightUp(wxMouseEvent& evt);
  void HandleTabMouseLeftDClick(wxMouseEvent& evt);

  void OnRClickMenu(wxCommandEvent& evt);

  // Text window event handlers:
  void OnTextFileNameChange(wxCommandEvent& evt);
  void OnTextModifiedChange(wxCommandEvent& evt);

  TabList::iterator GetTabByPos(int pos_x);
  Tab* GetTabByWindow(wxWindow* window, size_t* index = NULL);

  void ActivatePage(TabList::iterator it);
  bool RemovePage(TabList::iterator it);

  TabList::iterator GetActiveTab();
  TabList::iterator GetTabByPage(const TextPage* page);

  int CalcTabBestSize(const wxString& label) const;

  void RefreshTabArea();

  void PostPageChangedEvent();

protected:
  editor::SharedTheme theme_;

  int tab_min_size_;
  int tab_default_size_;

  // Tab area free/available size.
  int free_size_;

  notebook::TabArea* tab_area_;
  notebook::PageArea* page_area_;
  wxSizer* page_sizer_;

  TabList tabs_;

  // The front tab is the current active tab.
  // A tab is moved to the front whenever it's activated.
  TabList active_age_list_;

  Tab* rclicked_tab_;

  bool batch_;
  bool need_resize_tabs_;
};

////////////////////////////////////////////////////////////////////////////////
// Events

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(kEvtNotebookPageChanged, 0)
END_DECLARE_EVENT_TYPES()

#define EVT_NOTEBOOK_PAGE_CHANGED(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::kEvtNotebookPageChanged, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

} // namespace jil

#endif // JIL_NOTEBOOK_H_
