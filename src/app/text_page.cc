#include "app/text_page.h"

#include "editor/text_buffer.h"
#include "editor/text_window.h"
#include "editor/wrap.h"

#include "app/i18n_strings.h"
#include "app/page_window.h"
#include "app/save.h"

namespace jil {

TextPage::TextPage(editor::TextBuffer* buffer)
    : page_window_(NULL)
    , buffer_(buffer) {
  state_ = new PageState();
}

TextPage::~TextPage() {
  wxDELETE(buffer_);
  wxDELETE(state_);
}

//------------------------------------------------------------------------------

bool TextPage::Page_HasFocus() const {
  assert(page_window_ != NULL);
  return page_window_->HasFocus();
}

void TextPage::Page_SetFocus() {
  assert(page_window_ != NULL);
  return page_window_->SetFocus();
}

void TextPage::Page_Activate(bool active) {
  assert(page_window_ != NULL);

  if (active) {
    page_window_->SetPage(this);
  } else {
    // Do nothing.
  }
}

void TextPage::Page_Close() {
  // Do nothing.
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

  if (buffer_->new_created()) {
    flags |= kNewCreated;
  }

  return flags;
}

//void TextPage::Page_EditMenu(wxMenu* menu) {
//  //------------------------------------
//
//  AppendMenuItem(menu, ID_MENU_EDIT_UNDO, kTrEditUndo);
//  AppendMenuItem(menu, ID_MENU_EDIT_REDO, kTrEditRedo);
//  menu->AppendSeparator();
//
//  //------------------------------------
//
//  AppendMenuItem(menu, ID_MENU_EDIT_CUT, kTrEditCut);
//  AppendMenuItem(menu, ID_MENU_EDIT_COPY, kTrEditCopy);
//  AppendMenuItem(menu, ID_MENU_EDIT_PASTE, kTrEditPaste);
//  menu->AppendSeparator();
//
//  //------------------------------------
//
//  wxMenu* indent_menu = new wxMenu;
//  AppendMenuItem(indent_menu, ID_MENU_EDIT_INCREASE_INDENT, kTrEditIncreaseIndent);
//  AppendMenuItem(indent_menu, ID_MENU_EDIT_DECREASE_INDENT, kTrEditDecreaseIndent);
//  AppendMenuItem(indent_menu, ID_MENU_EDIT_AUTO_INDENT, kTrEditAutoIndent);
//  menu->AppendSubMenu(indent_menu, kTrEditIndent);
//
//  //------------------------------------
//
//  wxMenu* comment_menu = new wxMenu;
//  AppendMenuItem(comment_menu, ID_MENU_EDIT_COMMENT, kTrEditComment);
//  AppendMenuItem(comment_menu, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
//  menu->AppendSubMenu(comment_menu, kTrEditComment);
//  menu->AppendSeparator();
//
//  //------------------------------------
//
//  AppendMenuItem(menu, ID_MENU_EDIT_FIND, kTrEditFind);
//  AppendMenuItem(menu, ID_MENU_EDIT_REPLACE, kTrEditReplace);
//  AppendMenuItem(menu, ID_MENU_EDIT_FIND_NEXT, kTrEditFindNext);
//  AppendMenuItem(menu, ID_MENU_EDIT_FIND_PREV, kTrEditFindPrev);
//  menu->AppendSeparator();
//
//  AppendMenuItem(menu, ID_MENU_EDIT_GO_TO, kTrEditGoTo);
//}

bool TextPage::Page_EditMenuState(int menu_id) {
  assert(page_window_ != NULL);
  return page_window_->GetEditMenuState(menu_id);
}

bool TextPage::Page_FileMenuState(int menu_id, wxString* text) {
  assert(page_window_ != NULL);
  return page_window_->GetFileMenuState(menu_id, text);
}

bool TextPage::Page_OnMenu(int menu_id) {
  assert(page_window_ != NULL);
  return page_window_->HandleMenu(menu_id);
}

bool TextPage::Page_Save() {
  assert(page_window_ != NULL);

  int code = ConfirmSave(this);

  if (code == wxCANCEL) {
    return false;
  }

  if (code == wxYES) {
    if (!Save(buffer(), page_window_)) {
      // Fail or cancel to save. Don't close.
      return false;
    }
  }

  // Saved successfully or the user chose not to save it.
  return true;
}

bool TextPage::Page_SaveAs() {
  return SaveBufferAs(buffer_, NULL);
}

//------------------------------------------------------------------------------

void TextPage::OnBufferLineChange(editor::LineChangeType type, const editor::LineChangeData& data) {
  if (state_->view_options.wrap && state_->wrap_helper != NULL) {
    if (type == editor::kLineAdded) {
      for (editor::Coord ln = data.first(); ln <= data.last(); ++ln) {
        state_->wrap_helper->UpdateLineWrap(ln);
      }
    } else if (type == editor::kLineUpdated) {
      for (editor::Coord ln = data.first(); ln <= data.last(); ++ln) {
        state_->wrap_helper->AddLineWrap(ln);
      }
    } else if (type == editor::kLineDeleted) {
      // NOTE: In reverse!
      for (editor::Coord ln = data.last(); ln >= data.first(); --ln) {
        state_->wrap_helper->RemoveLineWrap(ln);
      }
    }
  }
}

void TextPage::OnBufferChange(editor::ChangeType type) {
  // TODO: Might need to handle kTabOptionsChange.
}

//------------------------------------------------------------------------------

void TextPage::Attach() {
  buffer_->AttachListener(this);
}

void TextPage::Detach() {
  buffer_->DetachListener(this);
}

}  // namespace jil
