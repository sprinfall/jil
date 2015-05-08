#include "app/book_ctrl.h"
#include <cassert>
#include <memory>
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/wupdlock.h"
#include "base/math_util.h"
#include "editor/text_extent.h"
#include "editor/tip.h"
#include "app/i18n_strings.h"
#include "app/id.h"

namespace jil {

static const wxString kEllipsis = wxT("...");
static const wxString kStar = wxT("*");

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(BookTabArea, wxPanel)
EVT_SIZE                (BookTabArea::OnSize)
EVT_PAINT               (BookTabArea::OnPaint)
EVT_MOUSE_EVENTS        (BookTabArea::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST  (BookTabArea::OnMouseCaptureLost)
END_EVENT_TABLE()

BookTabArea::BookTabArea(BookCtrl* book_ctrl, wxWindowID id)
    : wxPanel(book_ctrl, id)
    , book_ctrl_(book_ctrl)
    , tip_handler_(NULL) {
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetCursor(wxCursor(wxCURSOR_ARROW));
}

BookTabArea::~BookTabArea() {
  if (tip_handler_ != NULL) {
    PopEventHandler();
    delete tip_handler_;
  }
}

void BookTabArea::SetToolTipEx(const wxString& tooltip) {
  if (tip_handler_ == NULL) {
    tip_handler_ = new editor::TipHandler(this);
    tip_handler_->set_start_on_move(true);
    PushEventHandler(tip_handler_);
  }
  tip_handler_->SetTip(tooltip);
}

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

////////////////////////////////////////////////////////////////////////////////

BookCtrl::BookCtrl(const editor::SharedTheme& theme)
    : theme_(theme) {
  Init();
}

BookCtrl::~BookCtrl() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    Disconnect((*it)->page->Page_Window()->GetId());
    delete (*it);
  }
  tabs_.clear();
}

bool BookCtrl::Create(wxWindow* parent, wxWindowID id) {
  assert(theme_);

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(BG));

  CreateTabArea();

  // Book ctrl's min size is the best size of its tab area.
  SetMinSize(tab_area_->GetBestSize());

  page_area_ = new wxPanel(this, wxID_ANY);
  page_vsizer_ = new wxBoxSizer(wxVERTICAL);
  page_area_->SetSizer(page_vsizer_);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(tab_area_, 0, wxEXPAND);
  vsizer->Add(page_area_, 1, wxEXPAND);
  SetSizer(vsizer);

  return true;
}

bool BookCtrl::HasFocus() const {
  if (wxPanel::HasFocus()) {
    return true;
  }

  BookPage* page = ActivePage();
  if (page != NULL && page->Page_Window()->HasFocus()) {
    return true;
  }

  return false;
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

  PostEvent(kEvtBookPageChange);
}

bool BookCtrl::RemovePage(const BookPage* page) {
  TabList::iterator it = TabByPage(page);
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool BookCtrl::RemoveActivePage() {
  TabList::iterator it = ActiveTab();
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool BookCtrl::RemoveAllPages(const BookPage* except_page) {
  wxWindowUpdateLocker avoid_flickering(this);

  bool any_page_removed = false;

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ) {
    Tab* tab = *it;
    if (tab->page == except_page) {
      ++it;
      continue;
    }

    if (!tab->page->Page_Close()) {
      // Failed to close page, stop removing.
      break;
    }

    // Reset right-clicked tab since the page will be destroyed.
    if (rclicked_tab_ != NULL && rclicked_tab_->page == tab->page) {
      rclicked_tab_ = NULL;
    }

    it = tabs_.erase(it);
    stack_tabs_.remove(tab);
    if (tab->active) {
      page_vsizer_->Clear(false);
    }
    delete tab;

    any_page_removed = true;
  }

  if (any_page_removed) {
    if (!stack_tabs_.empty()) {
      ActivatePage(stack_tabs_.front()->page);

      PostEvent(kEvtBookPageSwitch);
    }

    // The tab is removed, more space is available, resize the left tabs.
    ResizeTabs();

    tab_area_->Refresh();

    PostEvent(kEvtBookPageChange);
  }

  return (PageCount() == (except_page == NULL ? 0 : 1));
}

void BookCtrl::ActivatePage(BookPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      ActivatePage(it);
      return;
    }
  }
}

BookPage* BookCtrl::ActivePage() const {
  if (stack_tabs_.empty()) {
    return NULL;
  }
  if (!stack_tabs_.front()->active) {
    return NULL;
  }
  return stack_tabs_.front()->page;
}

void BookCtrl::SwitchToNextPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = tabs_.begin();
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

  TabList::iterator it = tabs_.begin();
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

#if 0
void BookCtrl::SwitchToNextStackPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = stack_tabs_.begin();
  ++it;
  ActivatePage((*it)->page);
}

void BookCtrl::SwitchToPrevStackPage() {
  if (PageCount() <= 1) {
    return;
  }

  ActivatePage(stack_tabs_.back()->page);
}
#endif

std::vector<BookPage*> BookCtrl::Pages() const {
  std::vector<BookPage*> pages;

  TabList::const_iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    pages.push_back((*it)->page);
  }

  return pages;
}

std::vector<BookPage*> BookCtrl::StackPages() const {
  std::vector<BookPage*> pages;

  TabList::const_iterator it = stack_tabs_.begin();
  for (; it != stack_tabs_.end(); ++it) {
    pages.push_back((*it)->page);
  }

  return pages;
}

BookPage* BookCtrl::NextPage(const BookPage* page) const {
  if (tabs_.size() <= 1) {
    return NULL;
  }

  TabList::const_iterator it = TabByPage(page);
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

void BookCtrl::Init() {
  char_width_ = 0;
  ellipsis_width_ = 0;

  tab_margin_top_ = 5;
  tab_area_padding_x_ = 3;

  tab_min_size_ = 10;
  tab_default_size_ = 130;

  free_size_ = 0;
  rclicked_tab_ = NULL;
  batch_ = false;
  need_resize_tabs_ = false;
}

void BookCtrl::CreateTabArea() {
  tab_area_ = new BookTabArea(this, wxID_ANY);

  if (theme_->GetColor(TAB_AREA_BG).IsOk()) {
    tab_area_->SetBackgroundColour(theme_->GetColor(TAB_AREA_BG));
  }

  if (tab_font_.IsOk()) {
    tab_area_->SetFont(tab_font_);
  }

  char_width_ = tab_area_->GetCharWidth();
  tab_area_->GetTextExtent(kEllipsis, &ellipsis_width_, NULL);
  tab_padding_.Set(char_width_, char_width_ / 2 + 1);
}

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

  const wxColour& tab_border = theme_->GetColor(TAB_BORDER);
  const wxColour& tab_bg = theme_->GetColor(TAB_BG);

  wxPen tab_pen = tab_border.IsOk() ? wxPen(tab_border) : *wxTRANSPARENT_PEN;
  wxBrush tab_brush = tab_bg.IsOk() ? wxBrush(tab_bg) : *wxTRANSPARENT_BRUSH;

  wxPen active_tab_pen(theme_->GetColor(ACTIVE_TAB_BORDER));
  wxBrush active_tab_brush(theme_->GetColor(ACTIVE_TAB_BG));

  dc.SetFont(tab_area_->GetFont());

  int x = rect.x;

  dc.SetPen(active_tab_pen);
  dc.DrawLine(x, bottom, x + tab_area_padding_x_, bottom);

  x += tab_area_padding_x_;

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it) {
    Tab* tab = *it;

    wxRect tab_rect(x, rect.y, tab->size, rect.height);
    tab_rect.y += tab_margin_top_;
    tab_rect.height -= tab_margin_top_;

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

    ColorId fg_color_id = tab->active ? ACTIVE_TAB_FG : TAB_FG;
    dc.SetTextForeground(theme_->GetColor(fg_color_id));

    wxRect tab_fg_rect = tab_rect;
    tab_fg_rect.Deflate(tab_padding_);
    tab_fg_rect.width -= char_width_;  // *

    wxString label = tab->page->Page_Label();

    if (!label.IsEmpty()) {
      if (tab->best_size <= tab->size) {
        dc.DrawText(label, tab_fg_rect.x, tab_fg_rect.y);
      } else {
        if (tab_fg_rect.width > ellipsis_width_) {
          int max_width = tab_fg_rect.width - ellipsis_width_;
          size_t i = editor::TailorLabel(dc, label, max_width);
          label = label.Mid(0, i) + kEllipsis;
          dc.DrawText(label, tab_fg_rect.x, tab_fg_rect.y);
        }
      }
    }

    if ((tab->page->Page_Flags() & BookPage::kModified) != 0) {
      int x = tab_fg_rect.GetRight() + char_width_ / 2;
      dc.DrawText(kStar, x, tab_fg_rect.y);
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

void BookCtrl::HandleTabMouseLeftDown(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseLeftUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }

  HandleTabMouseLeftUp(evt);

  tab_area_->ReleaseMouse();
}

void BookCtrl::HandleTabMouseLeftUp(wxMouseEvent& evt) {
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

  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    return;
  }

  RemovePage(it);
}

// Update tooltip on mouse motion.
void BookCtrl::OnTabMouseMotion(wxMouseEvent& evt) {
  wxString tooltip;

  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    tooltip = (*it)->page->Page_Description();
  }

  tab_area_->SetToolTipEx(tooltip);
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

void BookCtrl::HandleTabMouseRightDown(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseRightUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }
  tab_area_->ReleaseMouse();

  HandleTabMouseRightUp(evt);

  rclicked_tab_ = NULL;
}

void BookCtrl::HandleTabMouseRightUp(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseLeftDClick(wxMouseEvent& evt) {
  HandleTabMouseLeftDClick(evt);
}

void BookCtrl::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
}

BookCtrl::TabList::iterator BookCtrl::TabByPos(int pos_x) {
  int x = 0;

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if (pos_x >= x && pos_x < x + (*it)->size) {
      return it;
    }

    x += (*it)->size;
  }

  return tabs_.end();
}

BookCtrl::Tab* BookCtrl::GetTabByWindow(wxWindow* window, size_t* index) {
  size_t i = 0;

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it, ++i) {
    if ((*it)->page->Page_Window() == window) {
      if (index != NULL) {
        *index = i;
      }
      return (*it);
    }
  }

  return NULL;
}

void BookCtrl::ActivatePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return;
  }

  Tab* tab = *it;
  if (tab->active) {
    return;  // Already active
  }

  // NOTE: If freeze the page area, tab area and status bar can't be painted in GTK+.
  // Don't know the reason. (2015-05-04)
  //page_area_->Freeze();

  // Deactivate previous active page.
  TabList::iterator active_it = ActiveTab();
  if (active_it != tabs_.end()) {
    (*active_it)->active = false;
    (*active_it)->page->Page_Activate(false);
    page_vsizer_->Clear(false);
  }

  // Activate new page.
  (*it)->active = true;
  page_vsizer_->Add((*it)->page->Page_Window(), 1, wxEXPAND);
  (*it)->page->Page_Activate(true);

  // Make sure the active tab has enough space to display.
  if ((*it)->size < (*it)->best_size) {
    ResizeTabs();
  }

  // Update tab stack.
  stack_tabs_.remove(tab);
  stack_tabs_.push_front(tab);

  page_area_->Layout();
  //page_area_->Thaw();

  tab_area_->Refresh();

  PostEvent(kEvtBookPageSwitch);
}

bool BookCtrl::RemovePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;

  if (!tab->page->Page_Close()) {
    // Failed to close page, stop removing.
    return false;
  }

  // Reset right-clicked tab since the page will be destroyed.
  if (rclicked_tab_ != NULL && rclicked_tab_->page == tab->page) {
    rclicked_tab_ = NULL;
  }

  //page_area_->Freeze();

  tabs_.erase(it);
  stack_tabs_.remove(tab);

  // The page to remove is active.
  if (tab->active) {
    page_vsizer_->Clear(false);

    if (PageCount() > 0) {
      // Activate another page.
      Tab* active_tab = stack_tabs_.front();
      active_tab->active = true;
      page_vsizer_->Add(active_tab->page->Page_Window(), 1, wxEXPAND);
      active_tab->page->Page_Activate(true);

      PostEvent(kEvtBookPageSwitch);
    }
  }

  delete tab;

  page_area_->Layout();
  //page_area_->Thaw();

  // Resize tabs since more space is available.
  ResizeTabs();
  tab_area_->Refresh();

  PostEvent(kEvtBookPageChange);

  return true;
}

BookCtrl::TabList::iterator BookCtrl::ActiveTab() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      break;
    }
  }
  return it;
}

BookCtrl::TabList::iterator BookCtrl::TabByPage(const BookPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

BookCtrl::TabList::const_iterator BookCtrl::TabByPage(
    const BookPage* page) const {
  TabList::const_iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

int BookCtrl::CalcTabBestSize(const wxString& label) const {
  int label_size = 0;
  tab_area_->GetTextExtent(label, &label_size, NULL);
  // Add char_width_ for *.
  return label_size + tab_padding_.x + tab_padding_.x + char_width_;
}

wxSize BookCtrl::CalcTabAreaBestSize() const {
  int y = tab_area_->GetCharHeight();
  y += tab_margin_top_ + tab_padding_.y + tab_padding_.y;
  return wxSize(-1, y);
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
