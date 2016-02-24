#include "app/text_page.h"

#include "editor/text_buffer.h"
#include "editor/text_window.h"

#include "app/i18n_strings.h"

namespace jil {

TextPage::TextPage(editor::TextBuffer* buffer)
  : buffer_(buffer) {
  view_ = new editor::TextView();
}

TextPage::~TextPage() {
  wxDELETE(buffer_);
  wxDELETE(view_);
}

void TextPage::Page_Activate(bool active) {
  view_;
}

//wxString TextPage::Page_Type() const {
//  static const wxString kType = "TextPage";
//  return kType;
//}

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

}  // namespace jil
