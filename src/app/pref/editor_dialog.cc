#include "app/pref/editor_dialog.h"
#include "wx/notebook.h"

#include "app/pref/editor_general_page.h"
#include "app/pref/editor_indent_page.h"
#include "app/pref/editor_comment_page.h"

namespace jil {
namespace pref {

EditorDialog::EditorDialog(editor::Options* options)
    : options_(options) {
}

EditorDialog::~EditorDialog() {
}

void EditorDialog::AddPages() {
  notebook_->AddPage(CreateGeneralPage(), _("General"), true);
  notebook_->AddPage(CreateIndentPage(), _("Indent"), false);
  notebook_->AddPage(CreateCommentPage(), _("Comment"), false);
}

wxWindow* EditorDialog::CreateGeneralPage() {
  Editor_GeneralPage* page = new Editor_GeneralPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

wxWindow* EditorDialog::CreateIndentPage() {
  Editor_IndentPage* page = new Editor_IndentPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

wxWindow* EditorDialog::CreateCommentPage() {
  Editor_CommentPage* page = new Editor_CommentPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

}  // namespace pref
}  // namespace jil
