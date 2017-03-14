#include "app/text_book.h"

#include <cassert>
#include <memory>

#include "wx/clipbrd.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"

#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "editor/util.h"

#include "app/app.h"
#include "app/font_util.h"
#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/option.h"
#include "app/page_window.h"
#include "app/save.h"
#include "app/text_page.h"
#include "app/util.h"

namespace jil {

// TODO: Try to avoid dynamic_cast.
TextPage* AsTextPage(BookPage* page) {
  if (page != NULL) {
    return dynamic_cast<TextPage*>(page);
  }
  return NULL;
}

BEGIN_EVENT_TABLE(TextBook, BookCtrl)
EVT_TEXT_WINDOW(ID_TEXT_WINDOW, TextBook::OnTextWindowEvent)
END_EVENT_TABLE()

TextBook::TextBook() {
  Init();
}

TextBook::~TextBook() {
  //page_window_->SetPage(placeholder_page_);

  if (!tabs_.empty()) {
    for (Tab* tab : tabs_) {
      delete tab->page;
      delete tab;
    }
    tabs_.clear();
  }
}

bool TextBook::Create(wxWindow* parent, wxWindowID id) {
  assert(options_ != NULL);
  assert(style_ != NULL);
  assert(binding_ != NULL);
  assert(theme_ && page_theme_);

  if (!BookCtrl::Create(parent, id)) {
    return false;
  }

  CreatePageWindow();

  return true;
}

void TextBook::ReapplyTheme() {
  BookCtrl::ReapplyTheme();

  page_window_->ReapplyTheme();
  // TODO: Refresh inside page_window_->ReapplyTheme().
  page_window_->Refresh();
}

void TextBook::SetFocus() {
  if (!IsEmpty()) {
    page_window_->SetFocus();
  } else {
    wxPanel::SetFocus();
  }
}

TextPage* TextBook::AddPage(editor::TextBuffer* buffer, bool active) {
  TextPage* page = new TextPage(buffer);
  page->set_page_window(page_window_);
  BookCtrl::AddPage(page, active);
  return page;
}

TextPage* TextBook::ActiveTextPage() const {
  return AsTextPage(ActivePage());
}

std::vector<TextPage*> TextBook::TextPages() const {
  return TextPagesFromTabs(tabs_);
}

std::vector<TextPage*> TextBook::StackTextPages() const {
  return TextPagesFromTabs(stack_tabs_);
}

//------------------------------------------------------------------------------

void TextBook::DoActivateTab(Tab* tab, bool active) {
  if (active) {
    tab->active = true;
    tab->page->Page_Activate(true);

    if (!page_window_->IsShown()) {
      page_window_->Show();
      page_window_->SetFocus();
      page_panel_->Layout();
    }
  } else {
    tab->active = false;
    tab->page->Page_Activate(false);
  }
}

void TextBook::DoRemoveTab(Tab* tab, bool from_remove_all) {
  if (tab->active) {
    bool placeholder = false;

    if (!from_remove_all) {
      if (!IsEmpty()) {
        // Activate another page.
        Tab* next_active_tab = stack_tabs_.front();

        // NOTE: You can't just call ActivateTab(next_active_tab->page, true)!
        next_active_tab->active = true;
        next_active_tab->page->Page_Activate(true);
        MakeActiveTabVisible(true);  // Normally, should be already visible.

        PostEvent(kEvtBookPageSwitch);
      } else {
        placeholder = true;
      }
    } else {
      placeholder = true;
    }

    if (placeholder) {
      // Activate placeholder page and hide page window.
      // NOTE: Don't have to post kEvtBookPageSwitch.
      placeholder_page_->Page_Activate(true);

      page_window_->Hide();
      //page_panel_->Layout();
    }
  }

  delete tab->page;
}

//------------------------------------------------------------------------------

void TextBook::Init() {
  options_ = NULL;
  style_ = NULL;
  binding_ = NULL;

  placeholder_page_ = NULL;
  page_window_ = NULL;
}

void TextBook::CreatePageWindow() {
  CreatePlaceholderPage();

  page_window_ = new PageWindow(placeholder_page_);
  placeholder_page_->set_page_window(page_window_);

  page_window_->set_style(style_);
  page_window_->set_theme(page_theme_);
  page_window_->set_binding(binding_);

  page_window_->Create(page_panel_, ID_TEXT_WINDOW, true);
   
  // TODO: SetTextFont causes Refresh, avoid it.
  page_window_->SetTextFont(options_->fonts[FONT_TEXT]);
  page_window_->SetLinePadding(options_->line_padding);

  page_window_->set_font_range(editor::FontRange(kMinFontSize, kMaxFontSize));

  page_panel_->GetSizer()->Add(page_window_, 1, wxEXPAND);
}

void TextBook::CreatePlaceholderPage() {
  using namespace editor;

  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(kTxtFt);
  TextBuffer* buffer = new TextBuffer(kPlaceholderBufferId, ft_plugin, options_->file_encoding);

  // Never wrap line for placeholder page for performance.
  if (buffer->view_options().wrap) {
    ViewOptions view_options = buffer->view_options();
    view_options.wrap = false;
    buffer->set_view_options(view_options);
  }

  placeholder_page_ = new TextPage(buffer);
}

void TextBook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  wxPoint pos = evt.GetPosition();

  wxRect tab_rect;
  TabIter it = TabIterByPos(pos.x, &tab_rect);

  if (it != tabs_.end()) {
    wxRect close_icon_rect = GetTabCloseIconRect(tab_rect);
    if (close_icon_rect.Contains(pos)) {
      // Up on close icon, close the page.
      // If the buffer is modified, ask for save.
      BookPage* page = (*it)->page;

      if (page->Page_IsModified()) {
        if (!page->Page_Save()) {
          return;
        }
      } 

      RemovePage(it);
    }
  } else {
    if (evt.CmdDown()) {
      // Ctrl + LeftUp on the blank area, create a new page.
      wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_FILE_NEW);
      GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
    }
  }
}

void TextBook::HandleTabMouseMiddleUp(wxMouseEvent& evt) {
  TabIter it = TabIterByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    return;
  }

  BookPage* page = (*it)->page;

  // If the buffer is modified, ask for save.
  if (page->Page_IsModified()) {
    if (!page->Page_Save()) {
      return;
    }
  }

  RemovePage(it);
}

void TextBook::HandleTabMouseRightDown(wxMouseEvent& evt) {
  ActivatePageByPos(evt.GetPosition().x);
}

void TextBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  // Add menu items according to the text page clicked.
  // If a menu item shouldn't exist in the context, don't create it.
  wxMenu menu;

  BookPage* page = PageByPos(evt.GetPosition().x);

  if (page == NULL) {  // Click on blank area.
    menu.Append(ID_MENU_FILE_NEW, kTrRClickNewFile);

    if (PageCount() > 0) {
      menu.Append(ID_MENU_FILE_CLOSE_ALL, kTrRClickCloseAll);
    }
  } else {
    if (page->Page_IsModified()) {
      menu.Append(ID_MENU_FILE_SAVE, kTrRClickSave);
    }

    menu.Append(ID_MENU_FILE_CLOSE, kTrRClickClose);

    if (PageCount() > 1) {
      menu.Append(ID_MENU_FILE_CLOSE_ALL, kTrRClickCloseAll);
      menu.Append(ID_MENU_FILE_CLOSE_ALL_BUT_THIS, kTrRClickCloseAllButThis);
    }

    if (!page->Page_IsNewCreated()) {
      menu.AppendSeparator();
      menu.Append(ID_MENU_FILE_COPY_PATH, kTrRClickCopyFilePath);
      menu.Append(ID_MENU_FILE_OPEN_FOLDER, kTrRClickOpenFolder);
    }
  }

  PopupMenu(&menu, evt.GetPosition());
}

// Left double-click on blank area, new a file.
void TextBook::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
  TabIter it = TabIterByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_FILE_NEW);
    GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
  }
}

//------------------------------------------------------------------------------

std::vector<TextPage*> TextBook::TextPagesFromTabs(const TabList& tabs) const {
  std::vector<TextPage*> text_pages;
  TextPage* text_page = NULL;

  for (const Tab* tab : tabs) {
    text_page = AsTextPage(tab->page);
    if (text_page != NULL) {
      text_pages.push_back(text_page);
    }
  }

  return text_pages;
}

void TextBook::OnTextWindowEvent(wxCommandEvent& evt) {
  evt.Skip();  // Skip for book frame.

  using namespace editor;

  int type = evt.GetInt();

  if (type == TextWindow::kEncodingEvent) {
    // Book frame needs this to update encoding field of status bar.
    evt.Skip();
  } else if (type == TextWindow::kFileNameEvent) {
    ResizeActiveTab();
  } else if (type == TextWindow::kModifiedStateEvent) {
    // evt.GetClientData(), if not NULL, would be the non-active text page
    // which was modified by Replace All, for example.
    tab_panel_->Refresh();
  }
}

}  // namespace jil
