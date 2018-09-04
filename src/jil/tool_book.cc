#include "jil/tool_book.h"

#include "wx/menu.h"
#include "wx/panel.h"
#include "wx/sizer.h"

#include "jil/book_page.h"
#include "jil/i18n_strings.h"
#include "jil/id.h"

namespace jil {

BEGIN_EVENT_TABLE(ToolBook, BookCtrl)
EVT_MENU(ID_MENU_TB_CLOSE, ToolBook::OnMenuClose)
EVT_MENU(ID_MENU_TB_CLOSE_ALL, ToolBook::OnMenuCloseAll)
EVT_MENU(ID_MENU_TB_CLOSE_ALL_BUT_THIS, ToolBook::OnMenuCloseAllButThis)
END_EVENT_TABLE()

ToolBook::ToolBook() {
}

ToolBook::~ToolBook() {
  if (!tabs_.empty()) {
    for (Tab* tab : tabs_) {
      delete tab->page;
      delete tab;
    }
    tabs_.clear();
  }
}

bool ToolBook::Create(wxWindow* parent, wxWindowID id) {
  if (!BookCtrl::Create(parent, id)) {
    return false;
  }

  return true;
}

void ToolBook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  wxMenu menu;

  BookPage* page = PageByPos(evt.GetPosition().x);

  if (page == NULL) {  // Click on blank area.
    if (PageCount() > 0) {
      menu.Append(ID_MENU_TB_CLOSE_ALL, kTrRClickCloseAll);
    }
  } else {
    menu.Append(ID_MENU_TB_CLOSE, kTrRClickClose);

    if (PageCount() > 1) {
      menu.Append(ID_MENU_TB_CLOSE_ALL, kTrRClickCloseAll);
      menu.Append(ID_MENU_TB_CLOSE_ALL_BUT_THIS, kTrRClickCloseAllButThis);
    }
  }

  PopupMenu(&menu, evt.GetPosition());
}

void ToolBook::DoActivateTab(Tab* tab, bool active) {
  if (active) {
    tab->active = true;
    tab->page->Page_Activate(true);
    page_panel_->Layout();
  } else {
    tab->active = false;
    tab->page->Page_Activate(false);
  }
}

void ToolBook::DoRemoveTab(Tab* tab, bool from_remove_all) {
  tab->page->Page_Close();

  if (tab->active) {
    page_panel_->GetSizer()->Clear(false);

    // The page to remove is active; activate another page.
    if (!from_remove_all && !IsEmpty()) {
      stack_tabs_.front()->active = true;
      stack_tabs_.front()->page->Page_Activate(true);

      PostEvent(kEvtBookPageSwitch);
    }
  }

  page_panel_->Layout();
}

void ToolBook::OnMenuClose(wxCommandEvent& evt) {
  RemoveActivePage();
}

void ToolBook::OnMenuCloseAll(wxCommandEvent& evt) {
  RemoveAllPages(false);
}

void ToolBook::OnMenuCloseAllButThis(wxCommandEvent& evt) {
  RemoveAllPages(false, ActivePage());
}

}  // namespace jil
