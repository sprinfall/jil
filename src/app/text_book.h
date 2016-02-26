#ifndef JIL_TEXT_BOOK_H_
#define JIL_TEXT_BOOK_H_
#pragma once

#include <list>
#include <vector>

#include "wx/panel.h"
#include "wx/dcbuffer.h"

#include "editor/theme.h"

namespace jil {

class Options;
class PageWindow;
class TextPage;

////////////////////////////////////////////////////////////////////////////////
// Events

BEGIN_DECLARE_EVENT_TYPES()
// Events posted by text book:
DECLARE_EVENT_TYPE(kEvtTextBookPageChange, 0)  // Page(s) added or removed.
DECLARE_EVENT_TYPE(kEvtTextBookPageSwitch, 0)  // Page activated.
END_DECLARE_EVENT_TYPES()

////////////////////////////////////////////////////////////////////////////////

class TextBook;

namespace editor {
class Binding;
class Style;
class TextBuffer;
class TipHandler;
}  // namespace editor

class TextBookTabArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  TextBookTabArea(TextBook* book_ctrl, wxWindowID id);
  virtual ~TextBookTabArea();

  // Page header has no focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

  void SetToolTipEx(const wxString& tooltip);

protected:
  virtual wxSize DoGetBestSize() const;

  void OnSize(wxSizeEvent& evt);
  void OnPaint(wxPaintEvent& evt);
  void OnMouseEvents(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

private:
  TextBook* book_ctrl_;
  editor::TipHandler* tip_handler_;
};

////////////////////////////////////////////////////////////////////////////////

class TextBook : public wxPanel {
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

protected:
  class Tab {
  public:
    Tab(TextPage* _page, int _best_size, bool _active)
        : page(_page)
        , best_size(_best_size)
        , size(_best_size)
        , active(_active) {
    }

    ~Tab() {
      wxDELETE(page);
    }

    TextPage* page;

    // Tab size.
    int best_size;  // Size fitting the label
    int size;  // Display size

    bool active;
  };

  typedef std::list<Tab*> TabList;
  typedef TabList::iterator TabIter;
  typedef TabList::const_iterator TabConstIter;

public:
  TextBook();
  virtual ~TextBook();

  void set_options(Options* options) {
    options_ = options;
  }

  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  // TODO
  void set_page_theme(const editor::SharedTheme& page_theme) {
    page_theme_ = page_theme;
  }

  void set_style(editor::Style* style) {
    style_ = style;
  }

  void set_binding(editor::Binding* binding) {
    binding_ = binding;
  }

  bool Create(wxWindow* parent, wxWindowID id);

  // NOTE: Call after Create().
  void SetTabFont(const wxFont& tab_font);

  void ReapplyTheme();

  virtual bool HasFocus() const override;

  // Override to set focus to page window.
  virtual void SetFocus() override;

  PageWindow* page_window() {
    return page_window_;
  }

  // Set batch flag to avoid unnecessary resizing tabs and refresh.
  // Example:
  //   text_book->StartBatch();
  //   text_book->AddPage(...);
  //   text_book->AddPage(...);
  //   ...
  //   text_book->EndBatch();
  void StartBatch();
  void EndBatch();

  // NOTE: Don't check if the page is already in the book or not.
  void AddPage(TextPage* page, bool active);

  // Add a page for the buffer.
  // Return the new created page.
  TextPage* AddPage(editor::TextBuffer* buffer, bool active);

  bool RemovePage(const TextPage* page);

  bool RemoveActivePage();

  // The except page, if specified, won't be removed.
  void RemoveAllPages(const TextPage* except_page = NULL);

  size_t PageCount() const {
    return tabs_.size();
  }

  bool IsEmpty() const {
    return PageCount() == 0;
  }

  void ActivatePage(TextPage* page);

  TextPage* ActivePage() const;

  void SwitchToNextPage();
  void SwitchToPrevPage();

#if 0
  // Not used
  void SwitchToNextStackPage();
  void SwitchToPrevStackPage();
#endif

  int GetStackIndex(TextPage* page) const;

  void MovePageToStackFront(TextPage* page);

  std::vector<TextPage*> Pages() const;
  std::vector<TextPage*> StackPages() const;

  // Return the page next to the given page.
  TextPage* NextPage(const TextPage* page) const;

  // Recalculate the size for each tab.
  void ResizeTabs();

protected:
  void Init();
  void CreatePageWindow();

  // Update values determined by tab font.
  void UpdateTabFontDetermined();

  friend class TextBookTabArea;

  void OnTabSize(wxSizeEvent& evt);

  void OnTabPaint(wxDC& dc, wxPaintEvent& evt);

  void OnTabMouse(wxMouseEvent& evt);

  void OnTabMouseLeftDown(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftDown(wxMouseEvent& evt);

  void OnTabMouseLeftUp(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftUp(wxMouseEvent& evt);

  void OnTabMouseMiddleDown(wxMouseEvent& evt);
  void OnTabMouseMiddleUp(wxMouseEvent& evt);
  virtual void HandleTabMouseMiddleUp(wxMouseEvent& evt);

  void OnTabMouseMotion(wxMouseEvent& evt);

  void OnTabMouseRightDown(wxMouseEvent& evt);
  virtual void HandleTabMouseRightDown(wxMouseEvent& evt);

  void OnTabMouseRightUp(wxMouseEvent& evt);
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt);

  void OnTabMouseLeftDClick(wxMouseEvent& evt);
  virtual void HandleTabMouseLeftDClick(wxMouseEvent& evt);

  TabList::iterator TabByPos(int pos_x);

  void ActivatePage(TabList::iterator it);
  bool RemovePage(TabList::iterator it);

  void ActivatePageByPos(int pos_x);

  TabIter ActiveTab();
  TabIter TabByPage(const TextPage* page);
  TabConstIter TabByPage(const TextPage* page) const;

  int CalcTabBestSize(const wxString& label) const;

  wxSize CalcTabAreaBestSize() const;

  void PostEvent(wxEventType event_type);

  // Handle text window event to update and refresh tab area.
  // TODO: Rename
  void OnTextWindowEvent(wxCommandEvent& evt);

protected:
  Options* options_;

  editor::Style* style_;

  editor::SharedTheme theme_;
  editor::SharedTheme page_theme_;

  editor::Binding* binding_;

  TextBookTabArea* tab_area_;
  wxPanel* page_area_;

  // An empty page used as a placeholder when no text page is added.
  // This ensures that text window always has a valid buffer.
  TextPage* empty_page_;

  PageWindow* page_window_;

  int char_width_;
  int ellipsis_width_;  // Size of "...".

  int tab_margin_top_;
  wxSize tab_padding_;

  // The space before/after the first/last tab.
  int tab_area_padding_x_;

  int tab_min_size_;
  int tab_default_size_;

  // Tab area free/available size.
  int free_size_;

  TabList tabs_;

  // The front tab is the current active tab.
  // A tab is moved to the front when it's activated.
  TabList stack_tabs_;

  bool batch_;
  bool need_resize_tabs_;
};

}  // namespace jil

#define EVT_TEXT_BOOK_PAGE_CHANGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::kEvtTextBookPageChange, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#define EVT_TEXT_BOOK_PAGE_SWITCH(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::kEvtTextBookPageSwitch, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_TEXT_BOOK_H_
