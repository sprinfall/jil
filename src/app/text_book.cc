#include "app/text_book.h"
#include "wx/menu.h"
#include "wx/clipbrd.h"
#include "editor/text_buffer.h"
#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/text_page.h"
#include "app/util.h"

namespace jil {

BEGIN_EVENT_TABLE(TextBook, BookCtrl)
END_EVENT_TABLE()

TextBook::TextBook(const editor::SharedTheme& theme)
    : BookCtrl(theme) {
}

bool TextBook::Create(wxWindow* parent, wxWindowID id) {
  if (!BookCtrl::Create(parent, id)) {
    return false;
  }

  return true;
}

TextBook::~TextBook() {
}

void TextBook::AddPage(BookPage* page, bool active) {
  BookCtrl::AddPage(page, active);

  Connect(page->Page_Window()->GetId(),
          editor::kTextWindowEvent,
          wxCommandEventHandler(TextBook::OnTextWindowEvent));
}

TextPage* TextBook::ActiveTextPage() const {
  return AsTextPage(ActivePage());
}

std::vector<TextPage*> TextBook::TextPages() const {
  std::vector<TextPage*> text_pages;
  TextPage* text_page = NULL;

  TabList::const_iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    text_page = AsTextPage((*it)->page);
    if (text_page != NULL) {
      text_pages.push_back(text_page);
    }
  }

  return text_pages;
}

std::vector<TextPage*> TextBook::StackTextPages() const {
  std::vector<TextPage*> text_pages;
  TextPage* text_page = NULL;

  TabList::const_iterator it = stack_tabs_.begin();
  for (; it != stack_tabs_.end(); ++it) {
    text_page = AsTextPage((*it)->page);
    if (text_page != NULL) {
      text_pages.push_back(text_page);
    }
  }

  return text_pages;
}

void TextBook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    if (evt.CmdDown()) {
      wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_FILE_NEW);
      GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
    }
  }
}

void TextBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  // Add menu items according to the text page clicked.
  // Note: If a menu item shouldn't exist in the context, don't create it.
  wxMenu menu;

  BookPage* page = NULL;
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    page = (*it)->page;
  }

  if (page == NULL) {  // Click on blank area
    menu.Append(ID_MENU_FILE_NEW, kTrRClickNewFile);

    if (PageCount() > 0) {
      menu.Append(ID_MENU_FILE_CLOSE_ALL, kTrRClickCloseAll);
    }
  } else {
    if ((page->Page_Flags() & BookPage::kModified) != 0) {
      menu.Append(ID_MENU_FILE_SAVE, kTrRClickSave);
    }

    menu.Append(ID_MENU_FILE_CLOSE, kTrRClickClose);

    if (PageCount() > 1) {
      menu.Append(ID_MENU_FILE_CLOSE_ALL, kTrRClickCloseAll);
      menu.Append(ID_MENU_FILE_CLOSE_ALL_BUT_THIS, kTrRClickCloseAllButThis);
    }

    TextPage* text_page = AsTextPage(page);
    if (text_page != NULL && !text_page->buffer_new_created()) {
      menu.AppendSeparator();
      menu.Append(ID_MENU_FILE_COPY_PATH, kTrRClickCopyFilePath);
      menu.Append(ID_MENU_FILE_OPEN_FOLDER, kTrRClickOpenFolder);
    }
  }

  PopupMenu(&menu, evt.GetPosition());
}

// Left double-click on blank area, new a file.
void TextBook::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_FILE_NEW);
    GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
  }
}

void TextBook::OnTextWindowEvent(wxCommandEvent& evt) {
  evt.Skip();  // Skip for book frame.

  int type = evt.GetInt();

  if (type == editor::TextWindow::kEncodingEvent) {
    // Book frame needs this to update encoding field of status bar.
    evt.Skip();
  } else if (type == editor::TextWindow::kFileNameEvent) {
    // Update tab best size.
    wxWindow* page_window = wxDynamicCast(evt.GetEventObject(), wxWindow);
    if (page_window != NULL) {
      Tab* tab = GetTabByWindow(page_window);
      if (tab != NULL) {
        tab->best_size = CalcTabBestSize(tab->page->Page_Label());
      }
    }
    tab_area_->Refresh();
  } else if (type == editor::TextWindow::kModifiedEvent) {
    tab_area_->Refresh();
  }
}

}  // namespace jil
