#include "app/tool_book.h"
#include "wx/menu.h"
#include "app/i18n_strings.h"
#include "app/id.h"

namespace jil {

BEGIN_EVENT_TABLE(ToolBook, BookCtrl)
EVT_MENU_RANGE(ID_MENU_BOOK_RCLICK_BEGIN, \
               ID_MENU_BOOK_RCLICK_END - 1, \
               ToolBook::OnRClickMenu)
END_EVENT_TABLE()

ToolBook::ToolBook(const editor::SharedTheme& theme)
    : BookCtrl(theme) {
}

bool ToolBook::Create(wxWindow* parent, wxWindowID id) {
  if (!BookCtrl::Create(parent, id)) {
    return false;
  }

  return true;
}

ToolBook::~ToolBook() {
}

void ToolBook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    ActivatePage(it);
  }
}

void ToolBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  rclicked_tab_ = it != tabs_.end() ? *it : NULL;

  if (rclicked_tab_ != NULL) {
    wxMenu menu;
    menu.Append(ID_MENU_BOOK_RCLICK_CLOSE, kTrRClickClose);
    PopupMenu(&menu, evt.GetPosition());
  }
}

void ToolBook::OnRClickMenu(wxCommandEvent& evt) {
  if (evt.GetId() == ID_MENU_BOOK_RCLICK_CLOSE) {
    BookPage* page = rclicked_tab_page();
    if (page != NULL) {
      RemovePage(page);
    }
  }
}

}  // namespace jil
