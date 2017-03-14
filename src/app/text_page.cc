#include "app/text_page.h"

#include <algorithm>  // std::min

#include "wx/menu.h"

#include "editor/action.h"
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
  state_->view_options = buffer->view_options();

  // Listen to the changes from buffer.
  buffer_->AttachListener(this);
}

TextPage::~TextPage() {
  buffer_->DetachListener(this);

  wxDELETE(buffer_);
  wxDELETE(state_);
}

void TextPage::set_page_window(PageWindow* page_window) {
  assert(page_window != NULL);

  page_window_ = page_window;
}

//------------------------------------------------------------------------------

bool TextPage::Page_HasFocus() const {
  assert(page_window_ != NULL);

  return page_window_->HasFocus();
}

void TextPage::Page_SetFocus() {
  assert(page_window_ != NULL);

  page_window_->SetFocus();
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
  assert(page_window_ != NULL);
  return page_window_->GetEditMenu(menu);
}

bool TextPage::Page_EditMenuState(int menu_id) {
  assert(page_window_ != NULL);
  return page_window_->GetEditMenuState(menu_id);
}

bool TextPage::Page_FileMenuState(int menu_id) {
  assert(page_window_ != NULL);
  return page_window_->GetFileMenuState(menu_id);
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

//------------------------------------------------------------------------------

void TextPage::OnBufferLineChange(editor::LineChangeType type, const editor::LineChangeData& data) {
  // Update wrap info.
  bool wrap = state_->view_options.wrap && state_->wrap_helper != NULL;
  if (wrap) {
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

  // If the selection intersects with the changed line range, clear
  // the selection.
  if (!state_->selection.IsEmpty()) {
    if (data.IntersectWith(state_->selection.GetLineRange())) {
      state_->selection.Reset();
    }
  }

  // NOTE:
  // Leave caret point as it is.
  // You can update it if it's in one of the changed lines. But it's
  // not necessary.
}

void TextPage::OnBufferChange(editor::ChangeType type) {
  switch (type) {
  case editor::kModifiedStateChange:
    // Tell page window to send an event to its parent (TextBook) so that
    // the tab area could be refreshed.
    page_window_->OnPageModifiedStateChange(this);
    break;

  case editor::kViewOptionsChange:
    // Update the view options using buffer's view options.
    // This happens when the user changes the options in Preferences.
    state_->view_options = buffer_->view_options();
    break;

  default:
    break;
  }
}

//------------------------------------------------------------------------------

void TextPage::Attach() {
  buffer_->AttachListener(this);
}

void TextPage::Detach() {
  buffer_->DetachListener(this);
}

//------------------------------------------------------------------------------

void TextPage::Replace(const editor::TextRange& range,
                       const std::wstring& replace_str,
                       bool grouped) {
  assert(page_window_ != NULL);

  if (page_window_->page() == this) {  // Active page.
    page_window_->Replace(range, replace_str, grouped);
  } else {
    buffer_->UndoableReplace(range, replace_str, grouped);
  }
}

}  // namespace jil
