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
EVT_MENU_RANGE(ID_MENU_BOOK_RCLICK_BEGIN, \
               ID_MENU_BOOK_RCLICK_END - 1, \
               TextBook::OnRClickMenu)
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

bool TextBook::AddPage(BookPage* page, bool active) {
  BookCtrl::AddPage(page, active);

  Connect(page->Page_Window()->GetId(),
          editor::kTextWindowEvent,
          wxCommandEventHandler(TextBook::OnTextWindowEvent));

  return true;
}

TextPage* TextBook::ActiveTextPage() const {
  return AsTextPage(ActivePage());
}

void TextBook::HandleTabMouseLeftDown(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    ActivatePage(it);
  }
}

void TextBook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    if (evt.CmdDown()) {
      // New file.
      wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED,
                             ID_MENU_BOOK_RCLICK_NEW_FILE);
      // NOTE: Post the event to text book itself instead of its parent.
      GetEventHandler()->AddPendingEvent(cmd_evt);
    }
  }
}

void TextBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  // Add menu items according to the text page clicked.
  // Note: If a menu item shouldn't exist in the context, don't create it.
  wxMenu menu;

  TabList::iterator it = TabByPos(evt.GetPosition().x);
  rclicked_tab_ = it != tabs_.end() ? *it : NULL;
  BookPage* page = rclicked_tab_ != NULL ? rclicked_tab_->page : NULL;

  if (page == NULL) { // Click on blank area
    menu.Append(ID_MENU_BOOK_RCLICK_NEW_FILE, kTrRClickNewFile);

    if (PageCount() > 0) {
      menu.Append(ID_MENU_BOOK_RCLICK_CLOSE_ALL, kTrRClickCloseAll);
    }
  } else {
    if ((page->Page_Flags() & BookPage::kModified) != 0) {
      menu.Append(ID_MENU_BOOK_RCLICK_SAVE, kTrRClickSave);
    }

    menu.Append(ID_MENU_BOOK_RCLICK_CLOSE, kTrRClickClose);

    if (PageCount() > 1) {
      menu.Append(ID_MENU_BOOK_RCLICK_CLOSE_ALL, kTrRClickCloseAll);
      menu.Append(ID_MENU_BOOK_RCLICK_CLOSE_ALL_BUT_THIS,
                  kTrRClickCloseAllButThis);
    }

    TextPage* text_page = AsTextPage(page);
    if (text_page != NULL && !text_page->buffer_new_created()) {
      menu.AppendSeparator();
      menu.Append(ID_MENU_BOOK_RCLICK_COPY_PATH, kTrRClickCopyFilePath);
      menu.Append(ID_MENU_BOOK_RCLICK_OPEN_FOLDER, kTrRClickOpenFolder);
    }
  }

  PopupMenu(&menu, evt.GetPosition());
}

// Left double-click on blank area, new a file.
void TextBook::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED,
                           ID_MENU_BOOK_RCLICK_NEW_FILE);
    // NOTE: Post the event to text book itself instead of its parent.
    GetEventHandler()->AddPendingEvent(cmd_evt);
  }
}

// NOTE: ID_MENU_BOOK_RCLICK_NEW_FILE is not handled here.
void TextBook::OnRClickMenu(wxCommandEvent& evt) {
  // TODO: Non-text pages should also be able to have rclick menus.
  TextPage* text_page = AsTextPage(rclicked_tab_page());

  switch (evt.GetId()) {
  case ID_MENU_BOOK_RCLICK_SAVE:
    if (text_page != NULL) {
      text_page->SaveBuffer();
    }
    break;

  case ID_MENU_BOOK_RCLICK_CLOSE:
    if (text_page != NULL) {
      RemovePage(text_page);
    }
    break;

  case ID_MENU_BOOK_RCLICK_CLOSE_ALL:
    RemoveAllPages();
    break;

  case ID_MENU_BOOK_RCLICK_COPY_PATH:
    if (text_page != NULL) {
      if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(
            new wxTextDataObject(text_page->buffer()->file_path_name()));
        wxTheClipboard->Close();
      }
    }
    break;

  case ID_MENU_BOOK_RCLICK_CLOSE_ALL_BUT_THIS:
    if (text_page != NULL) {
      RemoveAllPages(text_page);
    }
    break;

  case ID_MENU_BOOK_RCLICK_OPEN_FOLDER:
    if (text_page != NULL) {
      ExploreFile(text_page->buffer()->file_path_name());
    }
    break;

  default:
    // Skip to let book frame handle ID_MENU_BOOK_RCLICK_NEW_FILE.
    evt.Skip();
    break;
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
    RefreshTabArea();
  } else if (type == editor::TextWindow::kModifiedEvent) {
    RefreshTabArea();
  }
}

}  // namespace jil
