#include "jil/pref/editor_dialog.h"
#include "wx/notebook.h"

#include "jil/pref/editor_general_page.h"
#include "jil/pref/editor_indent_page.h"
#include "jil/pref/editor_comment_page.h"

namespace jil {
namespace pref {

EditorDialog::EditorDialog(editor::Options* options)
    : options_(options) {
}

EditorDialog::~EditorDialog() {
}

void EditorDialog::AddPages() {
  Editor_GeneralPage* general_page = new Editor_GeneralPage(options_);
  Editor_IndentPage* indent_page = new Editor_IndentPage(options_);
  Editor_CommentPage* comment_page = new Editor_CommentPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    general_page->SetBackgroundColour(theme_bg_colour);
    indent_page->SetBackgroundColour(theme_bg_colour);
    comment_page->SetBackgroundColour(theme_bg_colour);
  }

  general_page->Create(notebook_);
  indent_page->Create(notebook_);
  comment_page->Create(notebook_);

  notebook_->AddPage(general_page, _("General"), true);
  notebook_->AddPage(indent_page, _("Indent"), false);
  notebook_->AddPage(comment_page, _("Comment"), false);
}

}  // namespace pref
}  // namespace jil
