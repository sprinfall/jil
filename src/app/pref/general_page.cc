#include "app/pref/general_page.h"
#include "app/pref/general_config_panel.h"

namespace jil {
namespace pref {

wxWindow* GeneralPage::CreateWindow(wxWindow *parent) {
  GeneralConfigPanel* panel = new GeneralConfigPanel(options_);
  panel->Create(parent);
  return panel;
}

}  // namespace pref
}  // namespace jil
