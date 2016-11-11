#include "app/book_ctrl.h"

#include <cassert>
#include <memory>

#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/wupdlock.h"

#include "base/math_util.h"
#include "ui/util.h"

#include "editor/text_extent.h"
#include "editor/tip.h"

#include "app/book_page.h"
#include "app/i18n_strings.h"
#include "app/id.h"

namespace jil {

static const wxString kStar = wxT("*");

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(BookTabArea, wxPanel)
EVT_SIZE                (BookTabArea::OnSize)
EVT_PAINT               (BookTabArea::OnPaint)
EVT_MOUSE_EVENTS        (BookTabArea::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST  (BookTabArea::OnMouseCaptureLost)
EVT_LEAVE_WINDOW        (BookTabArea::OnLeaveWindow)
END_EVENT_TABLE()

BookTabArea::BookTabArea(BookCtrl* book_ctrl, wxWindowID id)
    : wxPanel(book_ctrl, id)
#if defined(__WXMSW__)
    , book_ctrl_(book_ctrl)
    , tip_handler_(NULL) {
#else
    , book_ctrl_(book_ctrl) {
#endif

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetCursor(wxCursor(wxCURSOR_ARROW));
}

BookTabArea::~BookTabArea() {
#if defined(__WXMSW__)
  if (tip_handler_ != NULL) {
    PopEventHandler();
    delete tip_handler_;
  }
#endif
}

#if defined(__WXMSW__)
void BookTabArea::SetToolTipEx(const wxString& tooltip) {
  if (tip_handler_ == NULL) {
    tip_handler_ = new editor::TipHandler(this);
    tip_handler_->set_start_on_move(true);
    PushEventHandler(tip_handler_);
  }
  tip_handler_->SetTip(tooltip);
}
#endif

wxSize BookTabArea::DoGetBestSize() const {
  return book_ctrl_->CalcTabAreaBestSize();
}

void BookTabArea::OnSize(wxSizeEvent& evt) {
  book_ctrl_->OnTabSize(evt);
}

void BookTabArea::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);

//#if !wxALWAYS_NATIVE_DOUBLE_BUFFER (20150508: Set background even when double bufferred (on Mac).
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
//#endif

  book_ctrl_->OnTabPaint(dc, evt);
}

void BookTabArea::OnMouseEvents(wxMouseEvent& evt) {
  book_ctrl_->OnTabMouse(evt);
}

void BookTabArea::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Do nothing.
}

void BookTabArea::OnLeaveWindow(wxMouseEvent& evt) {
  book_ctrl_->OnTabLeaveWindow(evt);
}

////////////////////////////////////////////////////////////////////////////////

BookCtrl::BookCtrl() {
  Init();
}

BookCtrl::~BookCtrl() {
  for (Tab* tab : tabs_) {
    delete tab;
  }
  tabs_.clear();
}

bool BookCtrl::Create(wxWindow* parent, wxWindowID id) {
  assert(theme_);

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(COLOR_BG));

  tab_area_ = new BookTabArea(this, wxID_ANY);
  tab_area_->SetBackgroundColour(theme_->GetColor(COLOR_TAB_AREA_BG));

  page_area_ = new wxPanel(this, wxID_ANY);
  wxSizer* page_vsizer = new wxBoxSizer(wxVERTICAL);
  page_area_->SetSizer(page_vsizer);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(tab_area_, 0, wxEXPAND);
  vsizer->Add(page_area_, 1, wxEXPAND);
  SetSizer(vsizer);

  UpdateTabFontDetermined();

  return true;
}

bool BookCtrl::HasFocus() const {
  if (wxPanel::HasFocus()) {
    return true;
  }

  BookPage* page = ActivePage();
  if (page != NULL) {
    return page->Page_HasFocus();
  }

  return false;
}

void BookCtrl::SetFocus() {
  BookPage* page = ActivePage();
  if (page != NULL) {
    return page->Page_SetFocus();
  }
  wxPanel::SetFocus();
}

void BookCtrl::SetTabFont(const wxFont& tab_font) {
  assert(tab_area_ != NULL);

  if (tab_area_->SetFont(tab_font)) {
    UpdateTabFontDetermined();
  }
}

void BookCtrl::ReapplyTheme() {
  assert(theme_);

  SetBackgroundColour(theme_->GetColor(COLOR_BG));
  tab_area_->SetBackgroundColour(theme_->GetColor(COLOR_TAB_AREA_BG));

  tab_area_->Refresh();
}

void BookCtrl::StartBatch() {
  batch_ = true;
  tab_area_->Freeze();
}

void BookCtrl::EndBatch() {
  batch_ = false;
  tab_area_->Thaw();

  if (need_resize_tabs_) {
    ResizeTabs();
    need_resize_tabs_ = false;
    tab_area_->Refresh();
  }
}

void BookCtrl::AddPage(BookPage* page, bool active) {
  int tab_best_size = CalcTabBestSize(page->Page_Label());
  Tab* tab = new Tab(page, tab_best_size, false);
  tabs_.push_back(tab);

  // Try to avoid resizing tabs.
  int expected_size = wxMax(tab->best_size, tab_default_size_);

  if (expected_size <= free_size_) {
    tab->size = expected_size;
    free_size_ -= expected_size;
  } else {
    if (!batch_) {
      ResizeTabs();
    } else {
      need_resize_tabs_ = true;
    }
  }

  if (active) {
    ActivatePage(--tabs_.end());
  } else {
    stack_tabs_.push_back(tab);
  }

  // TODO: Avoid when batch is on.
  PostEvent(kEvtBookPageChange);
}

bool BookCtrl::RemovePage(const BookPage* page) {
  TabIter it = TabIterByPage(page);
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool BookCtrl::RemoveActivePage() {
  TabIter it = ActiveTabIter();
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

void BookCtrl::RemoveAllPages(bool from_destroy, const BookPage* except_page) {
  wxWindowUpdateLocker avoid_flickering(this);

  bool removed = false;

  for (TabIter it = tabs_.begin(); it != tabs_.end(); ) {
    Tab* tab = *it;

    if (tab->page == except_page) {
      ++it;
      continue;
    }

    if (hover_tab_ == tab) {
      hover_tab_ = NULL;
    }

    it = tabs_.erase(it);
    stack_tabs_.remove(tab);

    DoRemoveAll(tab);

    wxDELETE(tab);

    removed = true;
  }

  if (removed && !from_destroy) {
    if (!stack_tabs_.empty()) {
      ActivatePage(stack_tabs_.front()->page);
      PostEvent(kEvtBookPageSwitch);
    }

    ResizeTabs();
    tab_area_->Refresh();
    PostEvent(kEvtBookPageChange);
  }
}

void BookCtrl::ActivatePage(BookPage* page) {
  TabIter it = TabIterByPage(page);
  if (it != tabs_.end()) {
    ActivatePage(it);
  }
}

BookPage* BookCtrl::ActivePage() const {
  Tab* tab = ActiveTab();
  if (tab != NULL) {
    return tab->page;
  }
  return NULL;
}

void BookCtrl::SwitchToNextPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      ++it;
      if (it == tabs_.end()) {
        it = tabs_.begin();
      }
      ActivatePage(it);
      return;
    }
  }
}

void BookCtrl::SwitchToPrevPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      if (it == tabs_.begin()) {
        it = tabs_.end();
      }
      --it;
      ActivatePage(it);
      return;
    }
  }
}

int BookCtrl::GetStackIndex(BookPage* page) const {
  TabConstIter it = stack_tabs_.begin();
  for (int i = 0; it != stack_tabs_.end(); ++it, ++i) {
    if ((*it)->page == page) {
      return i;
    }
  }
  return wxNOT_FOUND;
}

void BookCtrl::MovePageToStackFront(BookPage* page) {
  TabConstIter it = stack_tabs_.begin();
  for (int i = 0; it != stack_tabs_.end(); ++it) {
    Tab* tab = *it;
    if (tab->page == page) {
      stack_tabs_.erase(it);
      stack_tabs_.push_front(tab);
      break;
    }
  }
}

std::vector<BookPage*> BookCtrl::Pages() const {
  std::vector<BookPage*> pages;
  for (const Tab* tab : tabs_) {
    pages.push_back(tab->page);
  }
  return pages;
}

std::vector<BookPage*> BookCtrl::StackPages() const {
  std::vector<BookPage*> pages;
  for (const Tab* tab : stack_tabs_) {
    pages.push_back(tab->page);
  }
  return pages;
}

BookPage* BookCtrl::NextPage(const BookPage* page) const {
  if (tabs_.size() <= 1) {
    return NULL;
  }

  TabConstIter it = TabIterByPage(page);
  if (it == tabs_.end()) {
    return NULL;
  }

  ++it;
  if (it == tabs_.end()) {
    return tabs_.front()->page;
  } else {
    return (*it)->page;
  }
}

void BookCtrl::ResizeTabs() {
  if (tabs_.empty()) {
    return;
  }

  free_size_ = 0;  // Reset

  int client_size = tab_area_->GetClientSize().x;
  client_size -= tab_area_padding_x_ + tab_area_padding_x_;

  int sum_best_size = 0;

  // Initialize.
  for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
    sum_best_size += (*it)->best_size;
    (*it)->size = (*it)->best_size;  // Reset tab size to its best size.
  }

  if (sum_best_size < client_size) {
    // Give more size to small tabs.

    TabList small_tabs;
    for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
      if ((*it)->size < tab_default_size_) {
        small_tabs.push_back(*it);
      }
    }

    int free_size = client_size - sum_best_size;

    while (!small_tabs.empty()) {
      int avg_free_size = free_size / static_cast<int>(small_tabs.size());
      free_size %= static_cast<int>(small_tabs.size());

      if (avg_free_size == 0) {
        // Give 1px to each small tab.
        for (TabIter it = small_tabs.begin(); it != small_tabs.end(); ++it) {
          if (free_size == 0) {
            break;
          }
          ++(*it)->size;
          --free_size;
        }
        break;
      }

      for (TabIter it = small_tabs.begin(); it != small_tabs.end(); ) {
        Tab* small_tab = *it;
        int needed_size = tab_default_size_ - small_tab->size;
        if (needed_size > avg_free_size) {
          small_tab->size += avg_free_size;
          ++it;
        } else {
          // This tab doesn't need that much size.
          small_tab->size = tab_default_size_;
          // Return extra free size back.
          free_size += avg_free_size - needed_size;
          // This tab is not small any more.
          it = small_tabs.erase(it);
        }
      }
    }  // while (!small_tabs.empty())

    // Save free size.
    free_size_ = free_size;
    wxLogDebug(wxT("ResizeTabs, free_size: %d"), free_size_);

  } else {  // sum_best_size >= client_size
    // Reduce the size of large tabs (except the active one).

    int lack_size = sum_best_size - client_size;

    for (int large_size = tab_default_size_;
         large_size > tab_min_size_ && lack_size > 0;
         large_size /= 2) {
      TabList large_tabs;
      for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
        Tab* large_tab = *it;
        if (large_tab->size > large_size && !large_tab->active) {
          large_tabs.push_back(large_tab);
        }
      }

      if (large_tabs.empty()) {
        continue;
      }

      while (!large_tabs.empty()) {
        int avg_lack_size = lack_size / large_tabs.size();
        lack_size %= large_tabs.size();

        if (avg_lack_size == 0) {
          // Take 1px from first "lack_size" number of large tabs.
          for (TabIter it = large_tabs.begin(); it != large_tabs.end(); ++it) {
            if (lack_size == 0) {
              break;
            }
            --(*it)->size;
            --lack_size;
          }
          break;
        }

        for (TabIter it = large_tabs.begin(); it != large_tabs.end(); ) {
          Tab* large_tab = *it;
          int givable_size = large_tab->size - large_size;
          if (givable_size > avg_lack_size) {
            large_tab->size -= avg_lack_size;
            ++it;
          } else {
            // This tab cannot give that much size. Give all it can give.
            large_tab->size = large_size;
            // Return extra lack size back.
            lack_size += avg_lack_size - givable_size;
            // This tab is not large any more.
            it = large_tabs.erase(it);
          }
        }
      }
    }
  }
}

void BookCtrl::ResizeActiveTab() {
  Tab* tab = ActiveTab();
  if (tab != NULL) {
    int new_best_size = CalcTabBestSize(tab->page->Page_Label());
    if (tab->best_size != new_best_size) {
      tab->best_size = new_best_size;
      ResizeTabs();
      tab_area_->Refresh();
    }
  }
}

void BookCtrl::Init() {
  char_width_ = 0;
  ellipsis_width_ = 0;

  tab_margin_top_ = 5;
  tab_area_padding_x_ = 3;

  tab_min_size_ = 10;
  tab_default_size_ = 130;

  free_size_ = 0;

  tab_area_ = NULL;
  page_area_ = NULL;

  hover_tab_ = NULL;
  hover_on_close_icon_ = false;

  batch_ = false;
  need_resize_tabs_ = false;
}

void BookCtrl::UpdateTabFontDetermined() {
  char_width_ = tab_area_->GetCharWidth();
  tab_area_->GetTextExtent(ui::kEllipsis, &ellipsis_width_, NULL);
  tab_padding_.Set(char_width_, char_width_ / 2 + 1);
  tab_space_x_ = char_width_ / 2 + 1;

  // Book ctrl's min size is the best size of its tab area.
  SetMinSize(tab_area_->GetBestSize());

  // Update tabs best size.
  for (Tab* tab : tabs_) {
    tab->best_size = CalcTabBestSize(tab->page->Page_Label());
  }

  ResizeTabs();

  Layout();
  //tab_area_->Refresh();
}

//------------------------------------------------------------------------------
// Tab area event handlers.

void BookCtrl::OnTabSize(wxSizeEvent& evt) {
  if (!batch_) {
    ResizeTabs();
    tab_area_->Refresh();
  }
  evt.Skip();
}

// Paint tab items.
void BookCtrl::OnTabPaint(wxDC& dc, wxPaintEvent& evt) {
  wxRect rect = tab_area_->GetClientRect();
  int bottom = rect.GetBottom();

  const wxColour& tab_border = theme_->GetColor(COLOR_TAB_BORDER);
  const wxColour& tab_bg = theme_->GetColor(COLOR_TAB_BG);

  wxPen tab_pen = tab_border.IsOk() ? wxPen(tab_border) : *wxTRANSPARENT_PEN;
  wxBrush tab_brush = tab_bg.IsOk() ? wxBrush(tab_bg) : *wxTRANSPARENT_BRUSH;

  wxPen active_tab_pen(theme_->GetColor(COLOR_ACTIVE_TAB_BORDER));
  wxBrush active_tab_brush(theme_->GetColor(COLOR_ACTIVE_TAB_BG));

  wxBitmap tab_close_bitmap = theme_->GetImage(IMAGE_TAB_CLOSE);

  dc.SetFont(tab_area_->GetFont());

  int x = rect.x;

  dc.SetPen(active_tab_pen);
  dc.DrawLine(x, bottom, x + tab_area_padding_x_, bottom);

  x += tab_area_padding_x_;

  for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
    Tab* tab = *it;

    wxRect tab_rect = GetTabRect(x, tab->size, rect);

    // Background

    wxRect tab_bg_rect = tab_rect;
    tab_bg_rect.Deflate(1, 0);
    tab_bg_rect.height += 1;  // + 1 to make the bottom line invisible.

    if (tab->active) {
      dc.SetPen(active_tab_pen);
      dc.SetBrush(active_tab_brush);
      dc.DrawLine(tab_rect.x, bottom, tab_rect.x + tab->size, bottom);
      dc.DrawRectangle(tab_bg_rect);
    } else {
      if (!tab_pen.IsTransparent() || !tab_brush.IsTransparent()) {
        dc.SetPen(tab_pen);
        dc.SetBrush(tab_brush);
        dc.DrawRectangle(tab_bg_rect);
      }
      dc.SetPen(active_tab_pen);
      dc.DrawLine(tab_rect.x, bottom, tab_rect.x + tab->size, bottom);
    }

    // Foreground

    ColorId fg_color_id = tab->active ? COLOR_ACTIVE_TAB_FG : COLOR_TAB_FG;
    dc.SetTextForeground(theme_->GetColor(fg_color_id));

    wxRect tab_fg_rect = tab_rect;
    tab_fg_rect.Deflate(tab_padding_);

    wxRect label_rect = tab_fg_rect;
    label_rect.width -= tab_close_bitmap.GetWidth();
    label_rect.width -= tab_space_x_;
    label_rect.width -= char_width_;  // Modified indicator
    label_rect.width -= tab_space_x_;

    wxString label = tab->page->Page_Label();

    // Label.
    if (!label.IsEmpty()) {
      if (tab->best_size <= tab->size) {
        dc.DrawText(label, label_rect.x, label_rect.y);
      } else {
        if (label_rect.width > ellipsis_width_) {
          int max_width = label_rect.width - ellipsis_width_;
          size_t i = ui::TailorLabel(dc, label, max_width);
          label = label.Mid(0, i) + ui::kEllipsis;
          dc.DrawText(label, label_rect.x, label_rect.y);
        }
      }
    }

    // Modified indicator.
    if (tab->page->Page_IsModified()) {
      int star_x = label_rect.GetRight() + tab_space_x_;
      dc.DrawText(kStar, star_x, label_rect.y);
    }

    // Close icon.
    if (tab->active || tab == hover_tab_) {
      wxRect close_rect = GetTabCloseIconRect(tab_rect);

      if (hover_on_close_icon_ && tab == hover_tab_) {
        wxColour hover_bg = theme_->GetColor(
            tab->active ? COLOR_ACTIVE_TAB_HOVER_BG : COLOR_TAB_HOVER_BG);
        dc.SetPen(wxPen(hover_bg));
        dc.SetBrush(wxBrush(hover_bg));
        dc.DrawRectangle(close_rect);
      }

      dc.DrawBitmap(tab_close_bitmap, close_rect.GetLeftTop(), true);
    }

    x += tab->size;
  }

  if (x < rect.GetRight()) {
    dc.SetPen(active_tab_pen);
    dc.DrawLine(x, bottom, rect.GetRight() + 1, bottom);
  }
}

void BookCtrl::OnTabMouse(wxMouseEvent& evt) {
  wxEventType evt_type = evt.GetEventType();

  if (evt_type == wxEVT_LEFT_DOWN) {
    OnTabMouseLeftDown(evt);
  } else if (evt_type == wxEVT_LEFT_UP) {
    OnTabMouseLeftUp(evt);
  } else if (evt_type == wxEVT_MOTION) {
    OnTabMouseMotion(evt);
  } else if (evt_type == wxEVT_RIGHT_DOWN) {
    OnTabMouseRightDown(evt);
  } else if (evt_type == wxEVT_RIGHT_UP) {
    OnTabMouseRightUp(evt);
  } else if (evt_type == wxEVT_MIDDLE_DOWN) {
    OnTabMouseMiddleDown(evt);
  } else if (evt_type == wxEVT_MIDDLE_UP) {
    OnTabMouseMiddleUp(evt);
  } else if (evt_type == wxEVT_LEFT_DCLICK) {
    OnTabMouseLeftDClick(evt);
  }

  evt.Skip();
}

void BookCtrl::OnTabMouseLeftDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseLeftDown(evt);
}

void BookCtrl::OnTabMouseLeftUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }

  tab_area_->ReleaseMouse();
  HandleTabMouseLeftUp(evt);
}

void BookCtrl::OnTabMouseMiddleDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }
}

void BookCtrl::OnTabMouseMiddleUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }

  tab_area_->ReleaseMouse();
  HandleTabMouseMiddleUp(evt);
}

// Update tooltip on mouse motion.
void BookCtrl::OnTabMouseMotion(wxMouseEvent& evt) {
  wxPoint pos = evt.GetPosition();

  wxRect tab_rect;
  Tab* tab = TabByPos(pos.x, &tab_rect);

  if (tab == NULL) {
    if (hover_tab_ != NULL) {
      hover_tab_ = NULL;
      hover_on_close_icon_ = false;
      tab_area_->Refresh();
    }

    SetTabTooltip(wxEmptyString);
    return;
  }

  SetTabTooltip(tab->page->Page_Description());

  bool refresh = false;

  if (hover_tab_ != tab) {
    hover_tab_ = tab;
    refresh = true;
  }

  bool on_close_icon = GetTabCloseIconRect(tab_rect).Contains(pos);
  if (hover_on_close_icon_ != on_close_icon) {
    hover_on_close_icon_ = on_close_icon;
    refresh = true;
  }

  if (refresh) {
    tab_area_->Refresh();
  }
}

void BookCtrl::OnTabMouseRightDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseRightDown(evt);
}

void BookCtrl::OnTabMouseRightUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }

  tab_area_->ReleaseMouse();
  HandleTabMouseRightUp(evt);
}

void BookCtrl::OnTabMouseLeftDClick(wxMouseEvent& evt) {
  HandleTabMouseLeftDClick(evt);
}

void BookCtrl::OnTabLeaveWindow(wxMouseEvent& evt) {
  if (hover_tab_ != NULL) {
    hover_tab_ = NULL;
    tab_area_->Refresh();
  }
}

void BookCtrl::HandleTabMouseLeftDown(wxMouseEvent& evt) {
  wxPoint pos = evt.GetPosition();

  wxRect tab_rect;
  TabIter it = TabIterByPos(pos.x, &tab_rect);

  if (it != tabs_.end()) {
    // If click on the close icon, don't activate the page.
    wxRect close_icon_rect = GetTabCloseIconRect(tab_rect);
    if (!close_icon_rect.Contains(pos)) {
      ActivatePage(it);
    }
  }
}

void BookCtrl::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  wxPoint pos = evt.GetPosition();

  wxRect tab_rect;
  TabIter it = TabIterByPos(pos.x, &tab_rect);

  if (it != tabs_.end() && tab_rect.Contains(pos)) {
    RemovePage(it);
  }
}

void BookCtrl::HandleTabMouseMiddleUp(wxMouseEvent& evt) {
  TabIter it = TabIterByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    RemovePage(it);
  }
}

void BookCtrl::HandleTabMouseRightDown(wxMouseEvent& evt) {
  ActivatePageByPos(evt.GetPosition().x);
}

void BookCtrl::SetTabTooltip(const wxString& tooltip) {
#if defined(__WXMSW__)
  tab_area_->SetToolTipEx(tooltip);
#else
  tab_area_->SetToolTip(tooltip);
#endif
}

//------------------------------------------------------------------------------

BookCtrl::TabIter BookCtrl::TabIterByPos(int pos_x, wxRect* tab_rect) {
  wxRect rect = tab_area_->GetClientRect();
  int x = rect.x + tab_area_padding_x_;

  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if (pos_x >= x && pos_x < x + (*it)->size) {
      if (tab_rect != NULL) {
        *tab_rect = GetTabRect(x, (*it)->size, rect);
      }
      return it;
    }

    x += (*it)->size;
  }

  return tabs_.end();
}

BookCtrl::Tab* BookCtrl::TabByPos(int pos_x, wxRect* tab_rect) {
  TabIter it = TabIterByPos(pos_x, tab_rect);
  if (it != tabs_.end()) {
    return *it;
  }
  return NULL;
}

BookPage* BookCtrl::PageByPos(int pos_x) {
  Tab* tab = TabByPos(pos_x);
  if (tab != NULL) {
    return tab->page;
  }
  return NULL;
}

void BookCtrl::ActivatePage(TabIter it) {
  if (it == tabs_.end()) {
    return;
  }

  Tab* tab = *it;
  if (tab->active) {
    return;  // Already active
  }

  // Deactivate previous active page.
  TabIter active_it = ActiveTabIter();
  if (active_it != tabs_.end()) {
    DoActivateTab(*active_it, false);
  }

  // Activate new page.
  DoActivateTab(*it, true);

  // Make sure the active tab has enough space to display.
  if ((*it)->size < (*it)->best_size) {
    ResizeTabs();
  }

  // Update tab stack.
  stack_tabs_.remove(tab);
  stack_tabs_.push_front(tab);

  tab_area_->Refresh();

  PostEvent(kEvtBookPageSwitch);
}

bool BookCtrl::RemovePage(TabIter it) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;

  if (hover_tab_ == tab) {
    hover_tab_ = NULL;
  }

  tabs_.erase(it);
  stack_tabs_.remove(tab);

  DoRemoveTab(tab);

  delete tab;

  // Resize tabs since more space is available.
  ResizeTabs();
  tab_area_->Refresh();

  PostEvent(kEvtBookPageChange);

  return true;
}

void BookCtrl::ActivatePageByPos(int pos_x) {
  TabIter it = TabIterByPos(pos_x);
  if (it != tabs_.end()) {
    ActivatePage(it);
  }
}

BookCtrl::Tab* BookCtrl::ActiveTab() const {
  if (stack_tabs_.empty()) {
    return NULL;
  }
  if (!stack_tabs_.front()->active) {
    return NULL;
  }
  return stack_tabs_.front();
}

BookCtrl::TabIter BookCtrl::ActiveTabIter() {
  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      break;
    }
  }
  return it;
}

BookCtrl::TabIter BookCtrl::TabIterByPage(const BookPage* page) {
  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

BookCtrl::TabConstIter BookCtrl::TabIterByPage(const BookPage* page) const {
  TabConstIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

int BookCtrl::CalcTabBestSize(const wxString& label) const {
  int x = 0;
  tab_area_->GetTextExtent(label, &x, NULL);

  x += tab_padding_.x;
  x += tab_space_x_;
  x += char_width_;  // Modified indicator.
  x += tab_space_x_;
  x += theme_->GetImage(IMAGE_TAB_CLOSE).GetWidth();
  x += tab_padding_.x;

  return x;
}

wxSize BookCtrl::CalcTabAreaBestSize() const {
  int y = tab_area_->GetCharHeight();
  y += tab_margin_top_ + tab_padding_.y + tab_padding_.y;
  return wxSize(-1, y);
}

wxRect BookCtrl::GetTabRect(int x, int width, const wxRect& tab_area_rect) {
  wxRect tab_rect(x, tab_area_rect.y, width, tab_area_rect.height);
  tab_rect.y += tab_margin_top_;
  tab_rect.height -= tab_margin_top_;
  return tab_rect;
}

wxRect BookCtrl::GetTabCloseIconRect(const wxRect& tab_rect) {
  const wxBitmap& tab_close_bitmap = theme_->GetImage(IMAGE_TAB_CLOSE);

  int x = tab_rect.GetRight() - tab_padding_.x - tab_close_bitmap.GetWidth();
  int y = tab_rect.y + (tab_rect.height - tab_close_bitmap.GetHeight()) / 2;

  return wxRect(x, y, tab_close_bitmap.GetWidth(), tab_close_bitmap.GetHeight());
}

void BookCtrl::PostEvent(wxEventType event_type) {
  wxCommandEvent evt(event_type, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

////////////////////////////////////////////////////////////////////////////////
// Define event types.

DEFINE_EVENT_TYPE(kEvtBookPageChange);
DEFINE_EVENT_TYPE(kEvtBookPageSwitch);

}  // namespace jil
