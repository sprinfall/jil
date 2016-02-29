#include "app/find_result_page.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "app/id.h"
#include "app/i18n_strings.h"
#include "app/save.h"

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
  wxDELETE(buffer_);
}

bool FindResultPage::Page_HasFocus() const {
  return HasFocus();
}

void FindResultPage::Page_SetFocus() {
  SetFocus();
}

void FindResultPage::Page_Activate(bool active) {
  wxSizer* parent_Sizer = GetParent()->GetSizer();
  if (active) {
    parent_Sizer->Add(this, 1, wxEXPAND);
    Show(true);
    SetFocus();
  } else {
    Show(false);
    parent_Sizer->Clear(false);
  }
}

void FindResultPage::Page_Close() {
  Destroy();
}

wxString FindResultPage::Page_Type() const {
  static const wxString kType = "FindResultPage";
  return kType;
}

wxString FindResultPage::Page_Label() const {
  return buffer_->file_name_object().GetName();  // No ext
}

wxString FindResultPage::Page_Description() const {
  return wxEmptyString;
}

int FindResultPage::Page_Flags() const {
  return 0;
}

//void FindResultPage::Page_EditMenu(wxMenu* menu) {
//  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
//  menu->AppendSeparator();
//  AppendMenuItem(menu, ID_MENU_EDIT_CLEAR_ALL, kTrEditClearAll);
//  menu->AppendSeparator();
//  AppendMenuItem(menu, ID_MENU_EDIT_GO_TO_LOCATION, kTrEditGoToLocation);
//}

bool FindResultPage::Page_EditMenuState(int menu_id) {
  if (menu_id == ID_MENU_EDIT_COPY || menu_id == ID_MENU_EDIT_CLEAR_ALL) {
    return !buffer_->IsEmpty();
  } else if (menu_id == ID_MENU_EDIT_GO_TO_LOCATION) {
    return (buffer_->Line(caret_point_.y)->id() != editor::kNpos);
  }

  return true;
}

bool FindResultPage::Page_FileMenuState(int menu_id, wxString* text) {
  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    if (text != NULL) {
      *text = wxString::Format(kTrFileSaveAsFormat, Page_Label());
    }
    return true;
  }

  return false;
}

bool FindResultPage::Page_OnMenu(int menu_id) {
  // Special handling for Clear All and Go To since they have no binding.
  if (menu_id == ID_MENU_EDIT_CLEAR_ALL) {
    buffer_->DeleteText(buffer_->range());
    UpdateCaretPoint(buffer_->point_begin(), false, true, false);
    return true;
  } else if (menu_id == ID_MENU_EDIT_GO_TO_LOCATION) {
    PostEvent(kLocalizeEvent);
    return true;
  }

  editor::TextFunc* text_func = binding_->GetTextFuncByMenu(menu_id);
  if (text_func != NULL) {
    text_func->Exec(this);
    return true;
  }

  return false;
}

bool FindResultPage::Page_Save() {
  return false;
}

bool FindResultPage::Page_SaveAs() {
  return SaveBufferAs(buffer_, NULL);
}

// Double-click goes to the line of this result.
void FindResultPage::HandleTextLeftDClick(wxMouseEvent& evt) {
  using namespace editor;

  Coord ln = CalcCaretPoint(evt.GetPosition(), false).y;
  if (ln > buffer_->LineCount()) {
    return;  // Click on the blank area.
  }

  PostEvent(kLocalizeEvent);
}

void FindResultPage::HandleTextRightUp(wxMouseEvent& evt) {
  wxMenu menu;
  menu.Append(ID_MENU_EDIT_COPY, kTrRClickCopy);
  menu.AppendSeparator();
  menu.Append(ID_MENU_EDIT_CLEAR_ALL, kTrRClickClearAll);
  menu.AppendSeparator();
  menu.Append(ID_MENU_EDIT_GO_TO_LOCATION, kTrRClickGoToLocation);

  wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
  pos = ScreenToClient(pos);
  PopupMenu(&menu, pos);
}

void FindResultPage::PostEvent(int event_type) {
  wxCommandEvent evt(kFindResultPageEvent, GetId());
  evt.SetEventObject(this);
  evt.SetInt(event_type);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace jil
