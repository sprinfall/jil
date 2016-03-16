#include "app/text_page.h"

#include "wx/menu.h"

#include "editor/text_buffer.h"
#include "editor/text_window.h"
#include "editor/wrap.h"

#include "app/defs.h"
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
  if (page_window_ != NULL) {
    return page_window_->HasFocus();
  }
  return false;
}

void TextPage::Page_SetFocus() {
  if (page_window_ != NULL) {
    page_window_->SetFocus();
  }
}

void TextPage::Page_Activate(bool active) {
  if (page_window_ == NULL) {
    return;
  }

  if (active) {
    page_window_->SetPage(this);
  } else {
    // Do nothing.
  }
}

void TextPage::Page_Close() {
  // Do nothing.
}

int TextPage::Page_Type() const {
  return TEXT_PAGE_TYPE;
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

void TextPage::Page_EditMenu(wxMenu* menu) {
  if (page_window_ != NULL) {
    return page_window_->GetEditMenu(menu);
  }
}

bool TextPage::Page_EditMenuState(int menu_id) {
  if (page_window_ != NULL) {
    return page_window_->GetEditMenuState(menu_id);
  }
  return false;
}

bool TextPage::Page_FileMenuState(int menu_id, wxString* text) {
  if (page_window_ != NULL) {
    return page_window_->GetFileMenuState(menu_id, text);
  }
  return false;
}

bool TextPage::Page_OnMenu(int menu_id) {
  if (page_window_ != NULL) {
    return page_window_->HandleMenu(menu_id);
  }
  return false;
}

bool TextPage::Page_Save() {
  if (page_window_ == NULL) {
    return false;
  }

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
