#include "app/pref/editor_page.h"
#include "app/pref/editor_config_panel.h"

namespace jil {
namespace pref {

wxWindow* EditorPage::CreateWindow(wxWindow *parent) {
  return new EditorConfigPanel(parent);
}

}  // namespace pref
}  // namespace jil