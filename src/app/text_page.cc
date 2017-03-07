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
  switch (type) {
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

void TextPage::InsertString(const editor::TextPoint& point,
                            const std::wstring& str,
                            bool grouped,
                            bool update_caret) {
  editor::InsertStringAction* isa = new editor::InsertStringAction(buffer_, point, str);

  isa->set_caret_point(state_->caret_point);
  isa->set_update_caret(update_caret);
  isa->set_grouped(grouped);

  Exec(isa);
}

void TextPage::DeleteRange(const editor::TextRange& range,
                           editor::TextDir dir,
                           bool rect,
                           bool grouped,
                           bool selected,
                           bool update_caret) {
  editor::DeleteRangeAction* dra = new editor::DeleteRangeAction(buffer_, range, dir, rect, selected);

  dra->set_caret_point(state_->caret_point);
  dra->set_update_caret(update_caret);
  dra->set_grouped(grouped);

  Exec(dra);
}

void TextPage::Replace(const editor::TextRange& range,
                       const std::wstring& replace_str,
                       bool grouped) {
  assert(page_window_ != NULL);

  if (page_window_->page() == this) {  // I'm the active page.
    page_window_->Replace(range, replace_str, grouped);
  } else {
    grouped = grouped && !replace_str.empty();

    DeleteRange(range, editor::kForward, false, grouped, false, false);

    if (!replace_str.empty()) {
      InsertString(range.point_begin(), replace_str, grouped, false);
    }

    if (!state_->selection.IsEmpty()) {
      if (!state_->selection.GetLineRange().Intersect(range.GetLineRange()).IsEmpty()) {
        state_->selection.Reset();
      }
    }
  }
}

void TextPage::Exec(editor::Action* action) {
  action = buffer_->AddAction(action);

  if (action != NULL) {
    UpdateAfterExec(action);
  }
}

void TextPage::UpdateAfterExec(editor::Action* action) {
  editor::RangeAction* ra = action->AsRangeAction();

  if (action->update_caret()) {
    UpdateCaretPointAfterAction(action->CaretPointAfterExec(), ra);
  }
}

void TextPage::UpdateCaretPointAfterAction(const editor::TextPoint& point, editor::RangeAction* ra) {
  bool vspace = false;

  if (ra != NULL && ra->rect()) {
    // Allow virtual spaces for rect range action.
    vspace = true;
  }

  UpdateCaretPoint(point, false, vspace);
}

void TextPage::UpdateCaretPoint(const editor::TextPoint& point, bool line_step, bool vspace) {
  editor::TextPoint p(point);

  // Adjust the new caret point.
  editor::Coord line_length = buffer_->LineLength(p.y);
  if (line_step) {
    // The caret point is changed by moving to previous/next line or GoTo(ln).
    p.x = std::min(state_->max_caret_x, line_length);
  } else {
    if (!vspace) {  // Virtual space not allowed.
      if (p.x > line_length) {
        p.x = line_length;
      }
    }
  }

  if (p == state_->caret_point) {
    return;
  }

  state_->caret_point = p;

  if (!line_step) {
    state_->max_caret_x = p.x;
  }
}

}  // namespace jil
