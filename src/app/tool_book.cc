#include "app/tool_book.h"
#include "wx/menu.h"
#include "app/i18n_strings.h"
#include "app/id.h"

namespace jil {

BEGIN_EVENT_TABLE(ToolBook, BookCtrl)
EVT_MENU(ID_MENU_FILE_CLOSE, ToolBook::OnMenuClose)
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

void ToolBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    wxMenu menu;
    menu.Append(ID_MENU_FILE_CLOSE, kTrRClickClose);
    PopupMenu(&menu, evt.GetPosition());
  }
}

void ToolBook::OnMenuClose(wxCommandEvent& evt) {
  RemoveActivePage();
}

}  // namespace jil
