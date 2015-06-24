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

void TextPage::Page_Close() {
  Destroy();
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
  if (!buffer_->file_name().IsEmpty()) {
    return buffer_->file_path_name();
  }
  return kTrPageUntitled;
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
  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_UNDO, kTrEditUndo);
  AppendMenuItem(menu, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu->AppendSeparator();

  //------------------------------------

  AppendMenuItem(menu, ID_MENU_EDIT_CUT, kTrEditCut);
  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
  AppendMenuItem(menu, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu->AppendSeparator();

  //------------------------------------

  wxMenu* indent_menu = new wxMenu;
  AppendMenuItem(indent_menu, ID_MENU_EDIT_INCREASE_INDENT, kTrEditIncreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_DECREASE_INDENT, kTrEditDecreaseIndent);
  AppendMenuItem(indent_menu, ID_MENU_EDIT_AUTO_INDENT, kTrEditAutoIndent);
  menu->AppendSubMenu(indent_menu, kTrEditIndent);

  //------------------------------------

  wxMenu* comment_menu = new wxMenu;
  AppendMenuItem(comment_menu, ID_MENU_EDIT_COMMENT, kTrEditComment);
  AppendMenuItem(comment_menu, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
  menu->AppendSubMenu(comment_menu, kTrEditComment);
  menu->AppendSeparator();

  //------------------------------------

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
      // TODO: The page label might be too long.
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
  if (page != NULL) {
    return wxDynamicCast(page->Page_Window(), TextPage);
  }
  return NULL;
}

}  // namespace jil
