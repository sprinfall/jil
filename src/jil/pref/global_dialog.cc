#include "jil/pref/global_dialog.h"
#include "wx/notebook.h"

#include "jil/pref/common.h"
#include "jil/pref/global_general_page.h"
#include "jil/pref/global_font_page.h"
#include "jil/pref/global_theme_page.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(GlobalDialog, DialogBase)
EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, GlobalDialog::OnNotebookPageChanged)
END_EVENT_TABLE()

GlobalDialog::GlobalDialog(Options* options)
    : options_(options) {
}

GlobalDialog::~GlobalDialog() {
}

void GlobalDialog::AddPages() {
  Global_GeneralPage* general_page = new Global_GeneralPage(options_);
  Global_FontPage* font_page = new Global_FontPage(options_);
  Global_ThemePage* theme_page = new Global_ThemePage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    general_page->SetBackgroundColour(theme_bg_colour);
    font_page->SetBackgroundColour(theme_bg_colour);
    theme_page->SetBackgroundColour(theme_bg_colour);
  }

  general_page->Create(notebook_);
  font_page->Create(notebook_);
  theme_page->Create(notebook_);

  notebook_->AddPage(general_page, _("General"), true);
  notebook_->AddPage(font_page, _("Font"), false);
  notebook_->AddPage(theme_page, _("Theme"), false);
}

void GlobalDialog::OnNotebookPageChanged(wxBookCtrlEvent& evt) {
  int selection = evt.GetSelection();
  if (selection == 1) {
    // Enumerate system fonts if necessary.
    wxWindow* page = notebook_->GetPage(1);
    Global_FontPage* font_page = dynamic_cast<Global_FontPage*>(page);
    if (font_page != NULL) {
      font_page->EnumerateFonts();
    }
  }
}

}  // namespace pref
}  // namespace jil
