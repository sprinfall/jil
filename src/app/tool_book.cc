#include "app/tool_book.h"

#include "wx/menu.h"
#include "wx/panel.h"
#include "wx/sizer.h"

#include "app/book_page.h"
#include "app/i18n_strings.h"
#include "app/id.h"

namespace jil {

BEGIN_EVENT_TABLE(ToolBook, BookCtrl)
EVT_MENU(ID_MENU_FILE_CLOSE, ToolBook::OnMenuClose)
END_EVENT_TABLE()

ToolBook::ToolBook() {
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

void ToolBook::DoActivateTab(Tab* tab, bool active) {
  if (active) {
    tab->active = true;
    tab->page->Page_Activate(true);
    page_area_->Layout();
  } else {
    tab->active = false;
    tab->page->Page_Activate(false);
  }
}

void ToolBook::DoRemoveTab(Tab* tab) {
  tab->page->Page_Close();

  // The page to remove is active; activate another page.
  if (tab->active) {
    page_area_->GetSizer()->Clear(false);

    if (!IsEmpty()) {
      // TODO: Duplicate code as DoActivateTab().
      Tab* active_tab = stack_tabs_.front();
      active_tab->active = true;
      active_tab->page->Page_Activate(true);

      PostEvent(kEvtBookPageSwitch);
    }
  }

  page_area_->Layout();
}

void ToolBook::DoRemoveAll(Tab* tab) {
  tab->page->Page_Close();

  if (tab->active) {
    page_area_->GetSizer()->Clear(false);
  }
}


void ToolBook::OnMenuClose(wxCommandEvent& evt) {
  RemoveActivePage();
}

}  // namespace jil
