#include "app/text_page.h"
#include "wx/menu.h"
#include "wx/msgdlg.h"
#include "wx/log.h"
#include "editor/text_buffer.h"
#include "editor/text_area.h"
#include "editor/util.h"
#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/save.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(TextPage, editor::TextWindow)

TextPage::TextPage(editor::TextBuffer* buffer)
    : editor::TextWindow(buffer) {
}

TextPage::~TextPage() {
}

void TextPage::Page_Activate(bool active) {
  Show(active);
  SetFocus();
}

bool TextPage::Page_Close() {
  if (!buffer_->modified()) {
    Destroy();
    return true;
  }

  wxString confirm_msg;
  if (buffer_->new_created()) {
    confirm_msg = _("The file is untitled and changed, save it?");
  } else {
    confirm_msg = wxString::Format(
        _("The file (%s) has been changed, save it?"),
        Page_Label().c_str());
  }

  // Confirm save.
  int confirm_result = wxMessageBox(
      confirm_msg,
      _("Save File"),
      wxYES | wxNO | wxCANCEL | wxYES_DEFAULT | wxICON_EXCLAMATION | wxCENTRE);

  if (confirm_result == wxCANCEL) {
    // Don't close.
    return false;
  }

  if (confirm_result == wxNO) {
    // Don't save, close directly.
    Destroy();
    return true;
  }

  if (SaveBuffer()) {
    Destroy();
    return true;
  }

  return false;
}

wxString TextPage::Page_Type() const {
  static const wxString kType = "TextPage";
  return kType;
}

wxString TextPage::Page_Label() const {
  wxString label = buffer_->file_name();
  if (label.IsEmpty()) {
    label = kTrPageUntitled;
  }
  return label;
}

wxString TextPage::Page_Description() const {
  return buffer_->file_path_name();
}

int TextPage::Page_Flags() const {
  int flags = 0;

  if (buffer_->modified()) {
    flags |= kModified;
  }

  if (buffer_->file_name().IsEmpty()) {
    flags |= kUntitled;
  }

  return flags;
}

void TextPage::Page_EditMenu(wxMenu* menu) {
  AppendMenuItem(menu, ID_MENU_EDIT_UNDO, kTrEditUndo);
  AppendMenuItem(menu, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu->AppendSeparator();

  AppendMenuItem(menu, ID_MENU_EDIT_CUT, kTrEditCut);
  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
  AppendMenuItem(menu, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu->AppendSeparator();

  // - Line
  wxMenu* line_menu = new wxMenu;
  AppendMenuItem(line_menu,
                 ID_MENU_EDIT_AUTO_INDENT,
                 kTrEditAutoIndent);
  AppendMenuItem(line_menu,
                 ID_MENU_EDIT_INCREASE_INDENT,
                 kTrEditIncreaseIndent);
  AppendMenuItem(line_menu,
                 ID_MENU_EDIT_DECREASE_INDENT,
                 kTrEditDecreaseIndent);
  menu->AppendSubMenu(line_menu, kTrEditLine);

  // - Comment
  wxMenu* comment_menu = new wxMenu;
  AppendMenuItem(comment_menu, ID_MENU_EDIT_COMMENT, kTrEditComment);
  AppendMenuItem(comment_menu, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
  menu->AppendSubMenu(comment_menu, kTrEditComment);

  AppendMenuItem(menu, ID_MENU_EDIT_FIND, kTrEditFind);
  AppendMenuItem(menu, ID_MENU_EDIT_REPLACE, kTrEditReplace);
  AppendMenuItem(menu, ID_MENU_EDIT_GO_TO, kTrEditGoTo);
}

bool TextPage::Page_EditMenuState(int menu_id) {
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

bool TextPage::Page_FileMenuState(int menu_id, wxString* text) {
  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    if (text != NULL) {
      *text = wxString::Format(kTrFileSaveAsFormat, Page_Label());
    }
    return true;
  }

  return false;
}

bool TextPage::Page_OnMenu(int menu_id) {
  editor::TextFunc* text_func = binding_->GetTextFuncByMenu(menu_id);
  if (text_func != NULL) {
    text_func->Exec(this);
    return true;
  }
  return false;
}

void TextPage::Page_OnSaveAs() {
  SaveBufferAs(buffer_, NULL);
}

bool TextPage::SaveBuffer() {
  bool saved = false;
  if (buffer_->new_created() || buffer_->read_only()) {
    saved = SaveBufferAs(buffer_, wxGetTopLevelParent(this));
  } else {
    saved = ::jil::SaveBuffer(buffer_, wxGetTopLevelParent(this));
  }
  return saved;
}

void TextPage::HandleTextRightUp(wxMouseEvent& evt) {
  wxMenu menu;
  menu.Append(ID_MENU_EDIT_CUT, kTrRClickCut);
  menu.Append(ID_MENU_EDIT_COPY, kTrRClickCopy);
  menu.Append(ID_MENU_EDIT_PASTE, kTrRClickPaste);

  wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
  pos = ScreenToClient(pos);
  PopupMenu(&menu, pos);
}

////////////////////////////////////////////////////////////////////////////////

TextPage* AsTextPage(BookPage* page) {
  if (page == NULL) {
    return NULL;
  }
  return wxDynamicCast(page->Page_Window(), TextPage);
}

}  // namespace jil
