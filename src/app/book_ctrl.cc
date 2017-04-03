#include "app/book_ctrl.h"

#include <cassert>
#include <memory>

#include "wx/display.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/wupdlock.h"

#include "base/math_util.h"

#include "ui/bitmap_button.h"
#include "ui/util.h"

#include "editor/text_extent.h"
#include "editor/tip.h"

#include "app/book_page.h"
#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/popup_menu.h"
#include "app/util.h"

namespace jil {

static const wxString kStar = wxT("*");

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(TabPanel, wxPanel)
EVT_SIZE                (TabPanel::OnSize)
EVT_PAINT               (TabPanel::OnPaint)
EVT_MOUSE_EVENTS        (TabPanel::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST  (TabPanel::OnMouseCaptureLost)
EVT_LEAVE_WINDOW        (TabPanel::OnLeaveWindow)
END_EVENT_TABLE()

TabPanel::TabPanel(BookCtrl* book_ctrl, wxWindowID id)
    : wxPanel(book_ctrl, id)
#if defined(__WXMSW__)
    , tip_handler_(NULL)
#endif
    , book_ctrl_(book_ctrl) {

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetCursor(wxCursor(wxCURSOR_ARROW));
}

TabPanel::~TabPanel() {
#if defined(__WXMSW__)
  if (tip_handler_ != NULL) {
    PopEventHandler();
    delete tip_handler_;
  }
#endif
}

#if defined(__WXMSW__)
void TabPanel::SetToolTipEx(const wxString& tooltip) {
  if (tip_handler_ == NULL) {
    tip_handler_ = new editor::TipHandler(this);
    tip_handler_->set_start_on_move(true);
    PushEventHandler(tip_handler_);
  }
  tip_handler_->SetTip(tooltip);
}
#endif

wxSize TabPanel::DoGetBestSize() const {
  return book_ctrl_->CalcTabPanelBestSize();
}

void TabPanel::OnSize(wxSizeEvent& evt) {
  book_ctrl_->OnTabSize(evt);
}

void TabPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);

  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  book_ctrl_->OnTabPaint(dc, evt);
}

void TabPanel::OnMouseEvents(wxMouseEvent& evt) {
  book_ctrl_->OnTabMouse(evt);
}

void TabPanel::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Do nothing.
}

void TabPanel::OnLeaveWindow(wxMouseEvent& evt) {
  book_ctrl_->OnTabLeaveWindow(evt);
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(BookCtrl, wxPanel)
EVT_BUTTON(ID_TAB_EXPAND_BUTTON, BookCtrl::OnTabExpandButtonClick)
EVT_UPDATE_UI(ID_TAB_EXPAND_BUTTON, BookCtrl::OnTabExpandButtonUpdateUI)
EVT_POPUP_MENU_SELECT(BookCtrl::OnTabExpandMenuSelect)
END_EVENT_TABLE()

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
  assert(popup_theme_);

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(COLOR_BG));

  CreateTabPanel();

  page_panel_ = new wxPanel(this, wxID_ANY);
  wxSizer* page_vsizer = new wxBoxSizer(wxVERTICAL);
  page_panel_->SetSizer(page_vsizer);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(tab_panel_, 0, wxEXPAND);
  vsizer->Add(page_panel_, 1, wxEXPAND);
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
  assert(tab_panel_ != NULL);

  if (tab_panel_->SetFont(tab_font)) {
    UpdateTabFontDetermined();
  }
}

void BookCtrl::ReapplyTheme() {
  assert(theme_);

  SetBackgroundColour(theme_->GetColor(COLOR_BG));
  tab_panel_->SetBackgroundColour(theme_->GetColor(COLOR_TAB_PANEL_BG));

  tab_panel_->Refresh();
}

void BookCtrl::StartBatch() {
  batch_ = true;
  tab_panel_->Freeze();
}

void BookCtrl::EndBatch() {
  if (!batch_) {
    return;
  }

  batch_ = false;

  ResizeTabs();
  MakeActiveTabVisible(false);

  tab_panel_->Thaw();

  PostEvent(kEvtBookPageChange);
}

void BookCtrl::AddPage(BookPage* page, bool active) {
  int best_size = CalcTabBestSize(page->Page_Label());

  Tab* tab = new Tab(page, best_size, false);
  tabs_.push_back(tab);

  if (!batch_) {
    ResizeTabs();
  }

  if (active) {
    TabIter it = --tabs_.end();
    // If in batch more, the visibility of the active tab will be ensured
    // in EndBatch().
    bool ensure_visible = !batch_;
    ActivatePage(it, ensure_visible);
  } else {
    stack_tabs_.push_back(tab);
  }

  if (!batch_) {
    PostEvent(kEvtBookPageChange);
  }
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
  if (from_destroy && except_page != NULL) {
    except_page = NULL;  // Ignored
  }

  wxWindowUpdateLocker update_locker(this);

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

    DoRemoveTab(tab, true);

    wxDELETE(tab);
  }

  if (from_destroy) {
    // Since the book ctrl will be destroyed, no need to post events
    // or refresh.
    return;
  }

  if (!stack_tabs_.empty()) {  // except_page != NULL
    ActivatePage(stack_tabs_.front()->page, true);
  }

  // Post kEvtBookPageChange since pages' removed.
  PostEvent(kEvtBookPageChange);
}

void BookCtrl::ActivatePage(BookPage* page, bool ensure_visible) {
  TabIter it = TabIterByPage(page);
  if (it != tabs_.end()) {
    ActivatePage(it, ensure_visible);
  }
}

BookPage* BookCtrl::ActivePage() const {
  Tab* tab = ActiveTab();
  if (tab != NULL) {
    return tab->page;
  }
  return NULL;
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
  for (; it != stack_tabs_.end(); ++it) {
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
  visible_tabs_count_ = 0;

  if (tabs_.empty()) {
    return;
  }

  int client_size = tab_panel_->GetClientSize().x;
  client_size -= tab_panel_padding_x_;
  client_size -= theme_->GetImage(IMAGE_TAB_EXPAND).GetWidth();
  client_size -= tab_space_x_;
  client_size -= tab_panel_padding_x_;

  int sum_best_size = 0;

  for (Tab* tab : tabs_) {
    if (sum_best_size + tab->best_size > client_size) {
      break;  // Can't display more tabs.
    }

    sum_best_size += tab->best_size;
    tab->size = tab->best_size;  // Reset tab size to its best size.
    ++visible_tabs_count_;
  }

  if (visible_tabs_count_ == 0) {
    // The space can't display even one tab at its best size.
    // Give the whole space to this tab anyway. 
    tabs_.front()->size = client_size;
    visible_tabs_count_ = 1;
  } 
}

void BookCtrl::ResizeActiveTab() {
  Tab* tab = ActiveTab();
  if (tab != NULL) {
    int new_best_size = CalcTabBestSize(tab->page->Page_Label());
    if (tab->best_size != new_best_size) {
      tab->best_size = new_best_size;
      ResizeTabs();
      tab_panel_->Refresh();
    }
  }
}

void BookCtrl::Init() {
  char_width_ = 0;
  ellipsis_width_ = 0;

  tab_margin_top_ = 5;
  tab_panel_padding_x_ = 3;

  tab_panel_ = NULL;
  tab_expand_button_ = NULL;

  page_panel_ = NULL;

  visible_tabs_count_ = 0;

  hover_tab_ = NULL;
  hover_on_close_icon_ = false;

  batch_ = false;
}

void BookCtrl::CreateTabPanel() {
  tab_panel_ = new TabPanel(this, wxID_ANY);
  tab_panel_->SetBackgroundColour(theme_->GetColor(COLOR_TAB_PANEL_BG));

  ui::SharedButtonStyle button_style = ButtonStyleFromTheme(theme_->GetTheme(BookCtrl::THEME_BUTTON));
  tab_expand_button_ = new ui::BitmapButton(button_style);
  tab_expand_button_->Create(tab_panel_, ID_TAB_EXPAND_BUTTON);

  tab_expand_button_->SetBitmapsND(theme_->GetImage(BookCtrl::IMAGE_TAB_EXPAND),
                                   theme_->GetImage(BookCtrl::IMAGE_TAB_EXPAND_DISABLED));

  tab_expand_button_->set_click_type(ui::BitmapButton::kClickOnDown);

  // Layout
  wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->AddStretchSpacer(1);
  hsizer->AddSpacer(tab_space_x_);
  hsizer->Add(tab_expand_button_, wxSizerFlags().CenterVertical());
  hsizer->AddSpacer(tab_panel_padding_x_);
  tab_panel_->SetSizer(hsizer);
}

void BookCtrl::UpdateTabFontDetermined() {
  char_width_ = tab_panel_->GetCharWidth();
  tab_panel_->GetTextExtent(ui::kEllipsis, &ellipsis_width_, NULL);
  tab_padding_.Set(char_width_, char_width_ / 2 + 1);
  tab_space_x_ = char_width_ / 2 + 1;

  // Book ctrl's min size is the best size of its tab area.
  SetMinSize(tab_panel_->GetBestSize());

  // Update tabs best size.
  for (Tab* tab : tabs_) {
    tab->best_size = CalcTabBestSize(tab->page->Page_Label());
  }

  ResizeTabs();

  Layout();
  //tab_panel_->Refresh();
}

//------------------------------------------------------------------------------
// Tab area event handlers.

void BookCtrl::OnTabSize(wxSizeEvent& evt) {
  if (!batch_) {
    ResizeTabs();
    MakeActiveTabVisible(false);
    tab_panel_->Refresh();
  }
  evt.Skip();
}

// Paint tab items.
void BookCtrl::OnTabPaint(wxDC& dc, wxPaintEvent& evt) {
  wxRect rect = tab_panel_->GetClientRect();
  int bottom = rect.GetBottom();

  const wxColour& tab_border = theme_->GetColor(COLOR_TAB_BORDER);
  const wxColour& tab_bg = theme_->GetColor(COLOR_TAB_BG);

  wxPen tab_pen = tab_border.IsOk() ? wxPen(tab_border) : *wxTRANSPARENT_PEN;
  wxBrush tab_brush = tab_bg.IsOk() ? wxBrush(tab_bg) : *wxTRANSPARENT_BRUSH;

  wxPen active_tab_pen(theme_->GetColor(COLOR_ACTIVE_TAB_BORDER));
  wxBrush active_tab_brush(theme_->GetColor(COLOR_ACTIVE_TAB_BG));

  const wxBitmap& tab_close_bitmap = theme_->GetImage(IMAGE_TAB_CLOSE);

  dc.SetFont(tab_panel_->GetFont());

  int x = rect.x;

  // Draw the bottom line of the left pending area.
  dc.SetPen(active_tab_pen);
  dc.DrawLine(x, bottom, x + tab_panel_padding_x_, bottom);

  x += tab_panel_padding_x_;

  TabIter end_it = VisibleEndTabIter();
  for (TabIter it = tabs_.begin(); it != end_it; ++it) {
    Tab* tab = *it;

    wxRect tab_rect = GetTabRect(x, tab->size, rect);

    // Background

    wxRect tab_bg_rect = tab_rect;

    if (tab->active) {
      // Fill
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(active_tab_brush);
      dc.DrawRectangle(tab_bg_rect);

      // Borders (left, top and right, no bottom)
      dc.SetPen(active_tab_pen);
      dc.DrawLine(tab_rect.x, bottom, tab_rect.x, tab_rect.y);
      dc.DrawLine(tab_rect.x, tab_rect.y, tab_rect.GetRight(), tab_rect.y);
      dc.DrawLine(tab_rect.GetRight(), bottom, tab_rect.GetRight(), tab_rect.y);
    } else {
      if (!tab_pen.IsTransparent() || !tab_brush.IsTransparent()) {
        dc.SetPen(tab_pen);
        dc.SetBrush(tab_brush);
        dc.DrawRectangle(tab_bg_rect);
      }

      // Bottom line
      dc.SetPen(active_tab_pen);
      dc.DrawLine(tab_rect.x, bottom, tab_rect.GetRight(), bottom);
    }

    // Foreground

    ColorId fg_color_id = tab->active ? COLOR_ACTIVE_TAB_FG : COLOR_TAB_FG;
    dc.SetTextForeground(theme_->GetColor(fg_color_id));

    wxRect tab_fg_rect = tab_rect;
    tab_fg_rect.Deflate(tab_padding_);

    wxRect label_rect = tab_fg_rect;
    label_rect.width -= tab_close_bitmap.GetWidth();
    label_rect.width -= tab_space_x_;
    label_rect.width -= char_width_;  // Modify indicator
    label_rect.width -= tab_space_x_;

    wxString label = tab->page->Page_Label();

    // Label
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

    // Modify indicator
    if (tab->page->Page_IsModified()) {
      int star_x = label_rect.GetRight() + tab_space_x_;
      dc.DrawText(kStar, star_x, label_rect.y);
    }

    // Close icon
    if (tab->active || tab == hover_tab_) {
      wxRect close_icon_rect = GetTabCloseIconRect(tab_rect);

      if (hover_on_close_icon_ && tab == hover_tab_) {
        ColorId hover_color_id = tab->active ? COLOR_ACTIVE_TAB_HOVER_BG : COLOR_TAB_HOVER_BG;
        const wxColour& hover_bg = theme_->GetColor(hover_color_id);

        dc.SetPen(wxPen(hover_bg));
        dc.SetBrush(wxBrush(hover_bg));
        dc.DrawRectangle(close_icon_rect);
      }

      dc.DrawBitmap(tab_close_bitmap, close_icon_rect.GetLeftTop(), true);
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
  if (!tab_panel_->HasCapture()) {
    tab_panel_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseLeftDown(evt);
}

void BookCtrl::OnTabMouseLeftUp(wxMouseEvent& evt) {
  if (!tab_panel_->HasCapture()) {
    return;
  }

  tab_panel_->ReleaseMouse();
  HandleTabMouseLeftUp(evt);
}

void BookCtrl::OnTabMouseMiddleDown(wxMouseEvent& evt) {
  if (!tab_panel_->HasCapture()) {
    tab_panel_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }
}

void BookCtrl::OnTabMouseMiddleUp(wxMouseEvent& evt) {
  if (!tab_panel_->HasCapture()) {
    return;
  }

  tab_panel_->ReleaseMouse();
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
      tab_panel_->Refresh();
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
    tab_panel_->Refresh();
  }
}

void BookCtrl::OnTabMouseRightDown(wxMouseEvent& evt) {
  if (!tab_panel_->HasCapture()) {
    tab_panel_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseRightDown(evt);
}

void BookCtrl::OnTabMouseRightUp(wxMouseEvent& evt) {
  if (!tab_panel_->HasCapture()) {
    return;
  }

  tab_panel_->ReleaseMouse();
  HandleTabMouseRightUp(evt);
}

void BookCtrl::OnTabMouseLeftDClick(wxMouseEvent& evt) {
  HandleTabMouseLeftDClick(evt);
}

void BookCtrl::OnTabLeaveWindow(wxMouseEvent& evt) {
  if (hover_tab_ != NULL) {
    hover_tab_ = NULL;
    tab_panel_->Refresh();
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
      ActivatePage(it, false);
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
  tab_panel_->SetToolTipEx(tooltip);
#else
  tab_panel_->SetToolTip(tooltip);
#endif
}

void BookCtrl::OnTabExpandButtonClick(wxCommandEvent& evt) {
  if (visible_tabs_count_ >= tabs_.size()) {
    return;
  }

  std::vector<wxString> hidden_tabs;

  TabList::iterator it = tabs_.begin();
  std::advance(it, visible_tabs_count_);
  for (; it != tabs_.end(); ++it) {
    hidden_tabs.push_back((*it)->page->Page_Label());
  }

  jil::PopupMenu* popup_menu = new jil::PopupMenu(popup_theme_);

  wxRect owner_rect = tab_expand_button_->GetScreenRect();
  popup_menu->set_owner_rect(owner_rect);

  int display_index = wxDisplay::GetFromWindow(this);
  wxRect display_client_rect = wxDisplay(display_index).GetClientArea();
  popup_menu->set_display_client_rect(display_client_rect);

  popup_menu->Create(this, wxID_ANY);
  popup_menu->SetTabs(hidden_tabs);
  popup_menu->Popup();
}

void BookCtrl::OnTabExpandButtonUpdateUI(wxUpdateUIEvent& evt) {
  evt.Enable(IsToShowTabExpandButton());
}

void BookCtrl::OnTabExpandMenuSelect(PopupMenuEvent& evt) {
  if (visible_tabs_count_ >= tabs_.size() || visible_tabs_count_ == 0) {
    return;
  }

  TabIter it = TabIterByIndex(visible_tabs_count_ + evt.index());
  if (it == tabs_.end()) {
    return;
  }

  //it = MakeTabVisible(it);
  ActivatePage(it, true);

  // Get back the focus.
  SetFocus();
}

BookCtrl::TabIter BookCtrl::MakeTabVisible(TabIter it, bool check, bool refresh) {
  if (it == tabs_.end()) {
    return it;
  }

  if (check) {
    size_t index = std::distance(tabs_.begin(), it);
    if (index < visible_tabs_count_) {
      return it;  // Already visible.
    }
  }

  Tab* tab = *it;
  size_t count = visible_tabs_count_;

  while (true) {
    // Insert the tab AFTER the last visible tab.
    // NOTE: list.insert() inserts before the given position.
    tabs_.erase(it);
    it = tabs_.insert(TabIterByIndex(count), tab);

    // After resize tabs, visible_tabs_count_ might have be changed.
    ResizeTabs();

    if (visible_tabs_count_ > count) {
      // The tab does become visible.
      break;
    }

    if (count > 0) {
      --count;
    } else {
      break;
    }
  }

  tab_panel_->Refresh();

  return it;
}

BookCtrl::TabIter BookCtrl::MakeActiveTabVisible(bool refresh) {
  return MakeTabVisible(ActiveTabIter(), true, refresh);
}

//------------------------------------------------------------------------------

BookCtrl::TabIter BookCtrl::TabIterByIndex(size_t index) {
  if (index >= tabs_.size()) {
    return tabs_.end();
  }

  TabIter it = tabs_.begin();
  std::advance(it, index);
  return it;
}

BookCtrl::TabIter BookCtrl::TabIterByPos(int pos_x, wxRect* tab_rect) {
  wxRect rect = tab_panel_->GetClientRect();
  int x = rect.x + tab_panel_padding_x_;

  TabIter it = tabs_.begin();
  TabIter it_end = VisibleEndTabIter();

  for (; it != it_end; ++it) {
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

bool BookCtrl::ActivatePage(TabIter it, bool ensure_visible) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;
  if (tab->active) {
    return false;  // Already active
  }

  // Deactivate previous active page.
  TabIter active_it = ActiveTabIter();
  if (active_it != tabs_.end()) {
    DoActivateTab(*active_it, false);
  }

  // Activate new page.
  DoActivateTab(*it, true);

  // Make sure the new activated tab is visible.
  if (ensure_visible) {
    MakeTabVisible(it, true, false);
  }

  // Update tab stack.
  stack_tabs_.remove(tab);
  stack_tabs_.push_front(tab);

  tab_panel_->Refresh();

  PostEvent(kEvtBookPageSwitch);

  return true;
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

  DoRemoveTab(tab, false);

  wxDELETE(tab);

  // Resize tabs since more space is available.
  ResizeTabs();
  tab_panel_->Refresh();

  PostEvent(kEvtBookPageChange);

  return true;
}

void BookCtrl::ActivatePageByPos(int pos_x) {
  TabIter it = TabIterByPos(pos_x);
  if (it != tabs_.end()) {
    ActivatePage(it, false);
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

BookCtrl::TabIter BookCtrl::VisibleEndTabIter() {
  return TabIterByIndex(visible_tabs_count_);
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
  tab_panel_->GetTextExtent(label, &x, NULL);

  x += tab_padding_.x;
  x += tab_space_x_;
  x += char_width_;  // Modified indicator.
  x += tab_space_x_;
  x += theme_->GetImage(IMAGE_TAB_CLOSE).GetWidth();
  x += tab_padding_.x;

  return x;
}

wxSize BookCtrl::CalcTabPanelBestSize() const {
  int y = tab_panel_->GetCharHeight();
  y += tab_margin_top_ + tab_padding_.y + tab_padding_.y;
  return wxSize(-1, y);
}

wxRect BookCtrl::GetTabRect(int tab_x, int tab_width, const wxRect& tab_panel_rect) {
  wxRect tab_rect(tab_x, tab_panel_rect.y, tab_width, tab_panel_rect.height);
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
