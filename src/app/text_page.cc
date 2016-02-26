#include "app/text_page.h"

#include "editor/text_buffer.h"
#include "editor/text_window.h"
#include "editor/wrap.h"

#include "app/i18n_strings.h"

namespace jil {

TextPage::TextPage(editor::TextBuffer* buffer) : buffer_(buffer) {
  state_ = new PageState();
}

TextPage::~TextPage() {
  wxDELETE(buffer_);
  wxDELETE(state_);
}

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

wxString TextPage::GetLabel() const {
  wxString label = buffer_->file_name();
  if (label.IsEmpty()) {
    label = kTrPageUntitled;
  }
  return label;
}

wxString TextPage::GetDescription() const {
  if (!buffer_->file_name().IsEmpty()) {
    return buffer_->file_path_name();
  }
  return kTrPageUntitled;
}

int TextPage::GetFlags() const {
  int flags = 0;

  if (buffer_->modified()) {
    flags |= kModified;
  }

  if (buffer_->file_name().IsEmpty()) {
    flags |= kUntitled;
  }

  return flags;
}

void TextPage::Attach() {
  buffer_->AttachListener(this);
}

void TextPage::Detach() {
  buffer_->DetachListener(this);
}

}  // namespace jil
