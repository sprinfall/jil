#include "app/find_result_page.h"
#include "wx/menu.h"
#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "app/id.h"
#include "app/i18n_strings.h"

namespace jil {

DEFINE_EVENT_TYPE(kFindResultPageEvent);

IMPLEMENT_CLASS(FindResultPage, editor::TextWindow);

FindResultPage::FindResultPage(editor::TextBuffer* buffer)
    : editor::TextWindow(buffer) {
  allow_text_change_ = false;
}

bool FindResultPage::Create(wxWindow* parent, wxWindowID id, bool hide) {
  if (!editor::TextWindow::Create(parent, id, hide)) {
    return false;
  }
  return true;
}

FindResultPage::~FindResultPage() {
}

void FindResultPage::Page_Activate(bool active) {
  Show(active);
  SetFocus();
}

bool FindResultPage::Page_Close() {
  return Destroy();
}

wxString FindResultPage::Page_Type() const {
  static const wxString kType = "FindResultPage";
  return kType;
}

wxString FindResultPage::Page_Label() const {
  return _("Find Result");
}

wxString FindResultPage::Page_Description() const {
  return wxEmptyString;
}

int FindResultPage::Page_Flags() const {
  return 0;
}

void FindResultPage::Page_EditMenu(wxMenu* menu) {
  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
  menu->AppendSeparator();

  // TODO: Find Result Page should allow to find (not find all).
  AppendMenuItem(menu, ID_MENU_EDIT_FIND, kTrEditFind);
  AppendMenuItem(menu, ID_MENU_EDIT_GOTO, kTrEditGoto);
}

// Double-click goes to the line of this result.
void FindResultPage::HandleTextLeftDClick(wxMouseEvent& evt) {
  using namespace editor;

  Coord ln = CalcCaretPoint(evt.GetPosition(), false).y;
  if (ln > buffer_->LineCount()) {
    return;  // Click on the blank area.
  }

  wxCommandEvent fr_evt(kFindResultPageEvent, GetId());
  fr_evt.SetEventObject(this);
  fr_evt.SetInt(kLocalizeEvent);
  GetParent()->GetEventHandler()->AddPendingEvent(fr_evt);
}

void FindResultPage::HandleTextRightUp(wxMouseEvent& evt) {
  wxMenu menu;
  menu.Append(ID_MENU_EDIT_COPY, _("Copy"));

  wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
  pos = ScreenToClient(pos);
  PopupMenu(&menu, pos);
}

}  // namespace jil
