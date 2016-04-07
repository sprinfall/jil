#include "app/pref/global_dialog.h"
#include "wx/notebook.h"

#include "app/pref/global_general_page.h"
#include "app/pref/global_font_page.h"

namespace jil {
namespace pref {

GlobalDialog::GlobalDialog(Options* options)
    : options_(options) {
}

GlobalDialog::~GlobalDialog() {
}

void GlobalDialog::AddPages() {
  notebook_->AddPage(CreateGeneralPage(), _("General"), true);
  notebook_->AddPage(CreateFontPage(), _("Font"), false);
}

wxWindow* GlobalDialog::CreateGeneralPage() {
  Global_GeneralPage* page = new Global_GeneralPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

wxWindow* GlobalDialog::CreateFontPage() {
  Global_FontPage* page = new Global_FontPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

}  // namespace pref
}  // namespace jil
