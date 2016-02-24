#include "app/text_book.h"

#include <cassert>
#include <memory>

#include "wx/clipbrd.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/wupdlock.h"

#include "base/math_util.h"
#include "ui/util.h"

#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "editor/text_extent.h"
#include "editor/text_window.h"
#include "editor/tip.h"
#include "editor/util.h"

#include "app/app.h"
#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/option.h"
#include "app/save.h"
#include "app/text_page.h"
#include "app/util.h"

namespace jil {

static const wxString kStar = wxT("*");

////////////////////////////////////////////////////////////////////////////////
// Define event types.

DEFINE_EVENT_TYPE(kEvtTextBookPageChange);
DEFINE_EVENT_TYPE(kEvtTextBookPageSwitch);

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(PageWindow, editor::TextWindow)

PageWindow::PageWindow(TextPage* page)
    : editor::TextWindow(page->buffer())
    , page_(page) {
}

PageWindow::~PageWindow() {
}

void PageWindow::SetPage(TextPage* page) {
  assert(page != NULL);

  if (page_ != page) {
    // Save view state.
    GetView(page->view());

    page_ = page;

    // Set buffer and restore view state.
    SetBuffer(page->buffer(), page->view());
  }
}

void PageWindow::Page_EditMenu(wxMenu* menu) {
  if (page_ == NULL) {
    return;
  }

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_UNDO, kTrEditUndo);
  AppendMenuItem(menu, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu->AppendSeparator();

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_CUT, kTrEditCut);
  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
  AppendMenuItem(menu, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu->AppendSeparator();

  //------------------------------------

  wxMenu* indent_menu = new wxMenu;
  AppendMenuItem(indent_menu, ID_MENU_EDIT_INCREASE_INDENT, kTrEditIncreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_DECREASE_INDENT, kTrEditDecreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_AUTO_INDENT, kTrEditAutoIndent);
  menu->AppendSubMenu(indent_menu, kTrEditIndent);

  //------------------------------------

  wxMenu* comment_menu = new wxMenu;
  AppendMenuItem(comment_menu, ID_MENU_EDIT_COMMENT, kTrEditComment);
  AppendMenuItem(comment_menu, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
  menu->AppendSubMenu(comment_menu, kTrEditComment);
  menu->AppendSeparator();

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_FIND, kTrEditFind);
  AppendMenuItem(menu, ID_MENU_EDIT_REPLACE, kTrEditReplace);
  AppendMenuItem(menu, ID_MENU_EDIT_FIND_NEXT, kTrEditFindNext);
  AppendMenuItem(menu, ID_MENU_EDIT_FIND_PREV, kTrEditFindPrev);
  menu->AppendSeparator();

  AppendMenuItem(menu, ID_MENU_EDIT_GO_TO, kTrEditGoTo);
}

bool PageWindow::Page_EditMenuState(int menu_id) {
  if (page_ == NULL) {
    return false;
  }

  switch (menu_id) {
    case ID_MENU_EDIT_UNDO:
      return CanUndo();

    case ID_MENU_EDIT_REDO:
      return CanRedo();

    case ID_MENU_EDIT_PASTE:
      return !editor::IsClipboardEmpty();

    default:
      return true;
  }
}

bool PageWindow::Page_FileMenuState(int menu_id, wxString* text) {
  if (page_ == NULL) {
    return false;
  }

  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    if (text != NULL) {
      // TODO: The page label might be too long.
      *text = wxString::Format(kTrFileSaveAsFormat, page_->Page_Label());
    }
    return true;
  }

  return false;
}

bool PageWindow::Page_OnMenu(int menu_id) {
  if (page_ == NULL) {
    return false;
  }

  editor::TextFunc* text_func = binding_->GetTextFuncByMenu(menu_id);
  if (text_func != NULL) {
    text_func->Exec(this);
    return true;
  }

  return false;
}

void PageWindow::Page_OnSaveAs() {
  if (page_ == NULL) {
    return;
  }

  SaveBufferAs(buffer_, NULL);
}

void PageWindow::HandleTextRightUp(wxMouseEvent& evt) {
  if (page_ == NULL) {
    return;
  }

  wxMenu menu;
  menu.Append(ID_MENU_EDIT_CUT, kTrRClickCut);
  menu.Append(ID_MENU_EDIT_COPY, kTrRClickCopy);
  menu.Append(ID_MENU_EDIT_PASTE, kTrRClickPaste);

  // TODO: Add a method to TextWindow.
  wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
  pos = ScreenToClient(pos);
  PopupMenu(&menu, pos);
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(TextBookTabArea, wxPanel)
EVT_SIZE                (TextBookTabArea::OnSize)
EVT_PAINT               (TextBookTabArea::OnPaint)
EVT_MOUSE_EVENTS        (TextBookTabArea::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST  (TextBookTabArea::OnMouseCaptureLost)
END_EVENT_TABLE()

TextBookTabArea::TextBookTabArea(TextBook* book_ctrl, wxWindowID id)
    : wxPanel(book_ctrl, id)
    , book_ctrl_(book_ctrl)
    , tip_handler_(NULL) {
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetCursor(wxCursor(wxCURSOR_ARROW));
}

TextBookTabArea::~TextBookTabArea() {
  if (tip_handler_ != NULL) {
    PopEventHandler();
    delete tip_handler_;
  }
}

void TextBookTabArea::SetToolTipEx(const wxString& tooltip) {
  if (tip_handler_ == NULL) {
    tip_handler_ = new editor::TipHandler(this);
    tip_handler_->set_start_on_move(true);
    PushEventHandler(tip_handler_);
  }
  tip_handler_->SetTip(tooltip);
}

wxSize TextBookTabArea::DoGetBestSize() const {
  return book_ctrl_->CalcTabAreaBestSize();
}

void TextBookTabArea::OnSize(wxSizeEvent& evt) {
  book_ctrl_->OnTabSize(evt);
}

void TextBookTabArea::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);

//#if !wxALWAYS_NATIVE_DOUBLE_BUFFER (20150508: Set background even when double bufferred (on Mac).
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
//#endif

  book_ctrl_->OnTabPaint(dc, evt);
}

void TextBookTabArea::OnMouseEvents(wxMouseEvent& evt) {
  book_ctrl_->OnTabMouse(evt);
}

void TextBookTabArea::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////

TextBook::TextBook() {
  Init();
}

TextBook::~TextBook() {
  if (page_window_ != NULL) {
    Disconnect(page_window_->GetId());
  }

  editor::ClearContainer(&tabs_);
}

bool TextBook::Create(wxWindow* parent, wxWindowID id) {
  assert(options_ != NULL);
  assert(style_ != NULL);
  assert(binding_ != NULL);
  assert(theme_ && page_theme_);

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(BG));

  tab_area_ = new TextBookTabArea(this, wxID_ANY);
  tab_area_->SetBackgroundColour(theme_->GetColor(TAB_AREA_BG));

  page_area_ = new wxPanel(this, wxID_ANY);

  CreatePageWindow();
  
  wxSizer* page_vsizer = new wxBoxSizer(wxVERTICAL);
  page_vsizer->Add(page_window_, 1, wxEXPAND);
  page_area_->SetSizer(page_vsizer);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(tab_area_, 0, wxEXPAND);
  vsizer->Add(page_area_, 1, wxEXPAND);
  SetSizer(vsizer);

  UpdateTabFontDetermined();

  return true;
}

void TextBook::SetTabFont(const wxFont& tab_font) {
  assert(tab_area_ != NULL);

  if (tab_area_->SetFont(tab_font)) {
    UpdateTabFontDetermined();
  }
}

void TextBook::ReapplyTheme() {
  assert(theme_);

  SetBackgroundColour(theme_->GetColor(BG));
  tab_area_->SetBackgroundColour(theme_->GetColor(TAB_AREA_BG));

  tab_area_->Refresh();

  page_window_->ReapplyTheme();
  // TODO: Refresh inside page_window_->ReapplyTheme().
  page_window_->Refresh();
}

bool TextBook::HasFocus() const {
  return (wxPanel::HasFocus() || page_window_->HasFocus());
}

void TextBook::StartBatch() {
  batch_ = true;
  tab_area_->Freeze();
}

void TextBook::EndBatch() {
  batch_ = false;
  tab_area_->Thaw();

  if (need_resize_tabs_) {
    ResizeTabs();
    need_resize_tabs_ = false;
    tab_area_->Refresh();
  }
}

void TextBook::AddPage(TextPage* page, bool active) {
  if (PageCount() == 0) {
    // No pages added yet. Page window is hidden, show it.
    // TODO: Might be a little earlier, move to another place.
    page_window_->Show();
  }

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
  PostEvent(kEvtTextBookPageChange);
}

TextPage* TextBook::AddPage(editor::TextBuffer* buffer, bool active) {
  TextPage* page = new TextPage(buffer);
  AddPage(page, active);
  return page;
}

bool TextBook::RemovePage(const TextPage* page) {
  TabIter it = TabByPage(page);
  if (it != tabs_.end()) {
    return RemovePage(it);
  }
  return false;
}

bool TextBook::RemoveActivePage() {
  TabIter it = ActiveTab();
  if (it != tabs_.end()) {
    return RemovePage(it);
  }
  return false;
}

void TextBook::RemoveAllPages(const TextPage* except_page) {
  wxWindowUpdateLocker avoid_flickering(this);

  bool removed = false;

  for (TabIter it = tabs_.begin(); it != tabs_.end(); ) {
    Tab* tab = *it;

    if (tab->page == except_page) {
      ++it;
      continue;
    }

    it = tabs_.erase(it);
    stack_tabs_.remove(tab);

    if (tab->active) {
      page_window_->SetPage(empty_page_);
    }

    wxDELETE(tab);

    removed = true;
  }

  if (removed) {
    if (!stack_tabs_.empty()) {
      ActivatePage(stack_tabs_.front()->page);
      PostEvent(kEvtTextBookPageSwitch);
    }

    ResizeTabs();
    tab_area_->Refresh();
    PostEvent(kEvtTextBookPageChange);
  }
}

void TextBook::ActivatePage(TextPage* page) {
  TabIter it = TabByPage(page);
  if (it != tabs_.end()) {
    ActivatePage(it);
  }
}

TextPage* TextBook::ActivePage() const {
  if (stack_tabs_.empty()) {
    return NULL;
  }
  if (!stack_tabs_.front()->active) {
    return NULL;
  }
  return stack_tabs_.front()->page;
}

void TextBook::SwitchToNextPage() {
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

void TextBook::SwitchToPrevPage() {
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

#if 0
void TextBook::SwitchToNextStackPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabIter it = stack_tabs_.begin();
  ++it;
  ActivatePage((*it)->page);
}

void TextBook::SwitchToPrevStackPage() {
  if (PageCount() <= 1) {
    return;
  }

  ActivatePage(stack_tabs_.back()->page);
}
#endif

int TextBook::GetStackIndex(TextPage* page) const {
  TabConstIter it = stack_tabs_.begin();
  for (int i = 0; it != stack_tabs_.end(); ++it, ++i) {
    if ((*it)->page == page) {
      return i;
    }
  }
  return wxNOT_FOUND;
}

void TextBook::MovePageToStackFront(TextPage* page) {
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

std::vector<TextPage*> TextBook::Pages() const {
  std::vector<TextPage*> pages;
  for (TabConstIter it = tabs_.begin(); it != tabs_.end(); ++it) {
    pages.push_back((*it)->page);
  }
  return pages;
}

std::vector<TextPage*> TextBook::StackPages() const {
  std::vector<TextPage*> pages;
  for (TabConstIter it = stack_tabs_.begin(); it != stack_tabs_.end(); ++it) {
    pages.push_back((*it)->page);
  }
  return pages;
}

TextPage* TextBook::NextPage(const TextPage* page) const {
  if (tabs_.size() <= 1) {
    return NULL;
  }

  TabConstIter it = TabByPage(page);
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

void TextBook::ResizeTabs() {
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

void TextBook::Init() {
  options_ = NULL;
  style_ = NULL;
  binding_ = NULL;

  tab_area_ = NULL;
  page_area_ = NULL;

  empty_page_ = NULL;
  page_window_ = NULL;

  char_width_ = 0;
  ellipsis_width_ = 0;

  tab_margin_top_ = 5;
  tab_area_padding_x_ = 3;

  tab_min_size_ = 10;
  tab_default_size_ = 130;

  free_size_ = 0;

  batch_ = false;
  need_resize_tabs_ = false;
}

void TextBook::CreatePageWindow() {
  using namespace editor;

  // Create a buffer for the empty page.
  FileType txt_ft(kTxtFtId, kTrPlainText);
  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(txt_ft);
  TextBuffer* buffer = TextBuffer::Create(0, ft_plugin, options_->file_encoding);

  empty_page_ = new TextPage(buffer);

  page_window_ = new PageWindow(empty_page_);

  page_window_->set_style(style_);
  page_window_->set_theme(page_theme_);
  page_window_->set_binding(binding_);

  page_window_->Create(page_area_, wxID_ANY, true);

  page_window_->SetTextFont(options_->fonts[FONT_TEXT]);
  page_window_->SetLineNrFont(options_->fonts[FONT_LINE_NR]);
  page_window_->SetLinePadding(options_->line_padding);

  Connect(page_window_->GetId(),
          kTextWindowEvent,
          wxCommandEventHandler(TextBook::OnTextWindowEvent));
}

void TextBook::UpdateTabFontDetermined() {
  char_width_ = tab_area_->GetCharWidth();
  tab_area_->GetTextExtent(ui::kEllipsis, &ellipsis_width_, NULL);
  tab_padding_.Set(char_width_, char_width_ / 2 + 1);

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

void TextBook::OnTabSize(wxSizeEvent& evt) {
  if (!batch_) {
    ResizeTabs();
    tab_area_->Refresh();
  }
  evt.Skip();
}

// Paint tab items.
void TextBook::OnTabPaint(wxDC& dc, wxPaintEvent& evt) {
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

  for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
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
          size_t i = ui::TailorLabel(dc, label, max_width);
          label = label.Mid(0, i) + ui::kEllipsis;
          dc.DrawText(label, tab_fg_rect.x, tab_fg_rect.y);
        }
      }
    }

    if ((tab->page->Page_Flags() & TextPage::kModified) != 0) {
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

void TextBook::OnTabMouse(wxMouseEvent& evt) {
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

void TextBook::OnTabMouseLeftDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseLeftDown(evt);
}

void TextBook::HandleTabMouseLeftDown(wxMouseEvent& evt) {
  ActivatePageByPos(evt.GetPosition().x);
}

void TextBook::OnTabMouseLeftUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }

  HandleTabMouseLeftUp(evt);

  tab_area_->ReleaseMouse();
}

void TextBook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  TabIter it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    if (evt.CmdDown()) {
      wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_FILE_NEW);
      GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
    }
  }
}

void TextBook::OnTabMouseMiddleDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }
}

void TextBook::OnTabMouseMiddleUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }
  tab_area_->ReleaseMouse();

  HandleTabMouseMiddleUp(evt);
}

void TextBook::HandleTabMouseMiddleUp(wxMouseEvent& evt) {
  TabIter it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    return;
  }

  TextPage* page = (*it)->page;

  // If the buffer is modified, ask for save.
  if (page->buffer()->modified()) {
    int code = ConfirmSave(page);
    if (code == wxCANCEL) {
      return;  // Don't close.
    }
    if (code == wxYES) {
      if (!Save(page->buffer(), this)) {
        return;  // Fail or cancel to save. Don't close.
      }
    }
  }

  RemovePage(it);
}

// Update tooltip on mouse motion.
void TextBook::OnTabMouseMotion(wxMouseEvent& evt) {
  wxString tooltip;

  TabIter it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    tooltip = (*it)->page->Page_Description();
  }

  tab_area_->SetToolTipEx(tooltip);
}

void TextBook::OnTabMouseRightDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseRightDown(evt);
}

void TextBook::HandleTabMouseRightDown(wxMouseEvent& evt) {
  ActivatePageByPos(evt.GetPosition().x);
}

void TextBook::OnTabMouseRightUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }
  tab_area_->ReleaseMouse();

  HandleTabMouseRightUp(evt);
}

void TextBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  // Add menu items according to the text page clicked.
  // If a menu item shouldn't exist in the context, don't create it.
  wxMenu menu;

  TextPage* page = NULL;
  TabIter it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    page = (*it)->page;
  }

  if (page == NULL) {  // Click on blank area
    menu.Append(ID_MENU_FILE_NEW, kTrRClickNewFile);

    if (PageCount() > 0) {
      menu.Append(ID_MENU_FILE_CLOSE_ALL, kTrRClickCloseAll);
    }
  } else {
    if ((page->Page_Flags() & TextPage::kModified) != 0) {
      menu.Append(ID_MENU_FILE_SAVE, kTrRClickSave);
    }

    menu.Append(ID_MENU_FILE_CLOSE, kTrRClickClose);

    if (PageCount() > 1) {
      menu.Append(ID_MENU_FILE_CLOSE_ALL, kTrRClickCloseAll);
      menu.Append(ID_MENU_FILE_CLOSE_ALL_BUT_THIS, kTrRClickCloseAllButThis);
    }

    if (page != NULL && !page->buffer()->new_created()) {
      menu.AppendSeparator();
      menu.Append(ID_MENU_FILE_COPY_PATH, kTrRClickCopyFilePath);
      menu.Append(ID_MENU_FILE_OPEN_FOLDER, kTrRClickOpenFolder);
    }
  }

  PopupMenu(&menu, evt.GetPosition());
}

void TextBook::OnTabMouseLeftDClick(wxMouseEvent& evt) {
  HandleTabMouseLeftDClick(evt);
}

// Left double-click on blank area, new a file.
void TextBook::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
  TabIter it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_FILE_NEW);
    GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
  }
}

TextBook::TabIter TextBook::TabByPos(int pos_x) {
  int x = 0;

  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if (pos_x >= x && pos_x < x + (*it)->size) {
      return it;
    }

    x += (*it)->size;
  }

  return tabs_.end();
}

TextBook::Tab* TextBook::GetTabByWindow(wxWindow* window, size_t* index) {
  size_t i = 0;

  //for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it, ++i) {
  //  if ((*it)->page->Page_Window() == window) {
  //    if (index != NULL) {
  //      *index = i;
  //    }
  //    return (*it);
  //  }
  //}

  return NULL;
}

void TextBook::ActivatePage(TabIter it) {
  if (it == tabs_.end()) {
    return;
  }

  Tab* tab = *it;
  if (tab->active) {
    return;  // Already active
  }

  // Deactivate previous active page.
  TabIter active_it = ActiveTab();
  if (active_it != tabs_.end()) {
    // TODO: Add a method Activate(bool) to Tab.
    (*active_it)->active = false;
    (*active_it)->page->Page_Activate(false);
  }

  // Activate new page.
  tab->active = true;
  tab->page->Page_Activate(true);

  page_window_->SetPage(tab->page);

  // Make sure the active tab has enough space to display.
  if (tab->size < tab->best_size) {
    ResizeTabs();
  }

  // Update tab stack.
  stack_tabs_.remove(tab);
  stack_tabs_.push_front(tab);

  page_area_->Layout();  // TODO

  tab_area_->Refresh();

  PostEvent(kEvtTextBookPageSwitch);
}

bool TextBook::RemovePage(TabIter it) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;

  tabs_.erase(it);
  stack_tabs_.remove(tab);

  // The page to remove is active; activate another page.
  if (tab->active) {
    if (PageCount() > 0) {
      Tab* active_tab = stack_tabs_.front();

      active_tab->active = true;
      active_tab->page->Page_Activate(true);

      page_window_->SetPage(active_tab->page);

      PostEvent(kEvtTextBookPageSwitch);
    }
  }

  delete tab;

  page_area_->Layout();

  // Resize tabs since more space is available.
  ResizeTabs();
  tab_area_->Refresh();

  PostEvent(kEvtTextBookPageChange);

  return true;
}

void TextBook::ActivatePageByPos(int pos_x) {
  TabIter it = TabByPos(pos_x);
  if (it != tabs_.end()) {
    ActivatePage(it);
  }
}

TextBook::TabIter TextBook::ActiveTab() {
  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      break;
    }
  }
  return it;
}

TextBook::TabIter TextBook::TabByPage(const TextPage* page) {
  TabIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

TextBook::TabConstIter TextBook::TabByPage(const TextPage* page) const {
  TabConstIter it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

int TextBook::CalcTabBestSize(const wxString& label) const {
  int label_size = 0;
  tab_area_->GetTextExtent(label, &label_size, NULL);
  // Add char_width_ for *.
  return label_size + tab_padding_.x + tab_padding_.x + char_width_;
}

wxSize TextBook::CalcTabAreaBestSize() const {
  int y = tab_area_->GetCharHeight();
  y += tab_margin_top_ + tab_padding_.y + tab_padding_.y;
  return wxSize(-1, y);
}

void TextBook::PostEvent(wxEventType event_type) {
  wxCommandEvent evt(event_type, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

void TextBook::OnTextWindowEvent(wxCommandEvent& evt) {
  evt.Skip();  // Skip for book frame.

  int type = evt.GetInt();

  if (type == editor::TextWindow::kEncodingEvent) {
    // Book frame needs this to update encoding field of status bar.
    evt.Skip();
  } else if (type == editor::TextWindow::kFileNameEvent) {
    // TODO: Always come from current page?
    // Update tab best size.
    //wxWindow* page_window = wxDynamicCast(evt.GetEventObject(), wxWindow);
    //if (page_window != NULL) {
    //  Tab* tab = GetTabByWindow(page_window);
    //  if (tab != NULL) {
    //    tab->best_size = CalcTabBestSize(tab->page->Page_Label());
    //  }
    //}
    //tab_area_->Refresh();
  } else if (type == editor::TextWindow::kModifiedEvent) {
    tab_area_->Refresh();
  }
}

}  // namespace jil
