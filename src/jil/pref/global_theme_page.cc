#include "jil/pref/global_theme_page.h"

#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/stattext.h"

#include "editor/option.h"
#include "editor/util.h"

#include "jil/app.h"
#include "jil/defs.h"
#include "jil/option.h"
#include "jil/util.h"

#include "jil/pref/common.h"

namespace jil {
namespace pref {

Global_ThemePage::Global_ThemePage(Options* options)
    : options_(options) {
}

Global_ThemePage::~Global_ThemePage() {
}

bool Global_ThemePage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

bool Global_ThemePage::TransferDataToWindow() {
  theme_combo_box_->SetStringSelection(options_->theme);
  enlarge_icons_check_box_->SetValue(options_->icon_resolution == kHighResolution);
  return true;
}

bool Global_ThemePage::TransferDataFromWindow() {
  options_->theme = theme_combo_box_->GetValue();

  if (enlarge_icons_check_box_->IsChecked()) {
    options_->icon_resolution = kHighResolution;
  } else {
    options_->icon_resolution = kNormalResolution;
  }

  return true;
}

void Global_ThemePage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  //----------------------------------------------------------------------------
  // Color Scheme

  wxStaticText* theme_label = new wxStaticText(this, wxID_ANY, _("Color scheme:"));

  theme_combo_box_ = CreateReadonlyComboBox(this, wxID_ANY);

  App& app = wxGetApp();
  int count = wxMin(app.GetThemeCount(), kMaxThemes);
  for (int i = 0; i < count; ++i) {
    theme_combo_box_->Append(app.GetTheme(i));
  }

  LayoutField(top_vsizer, theme_label, theme_combo_box_);

  //----------------------------------------------------------------------------
  // Icon Resolution

  enlarge_icons_check_box_ = new wxCheckBox(this, wxID_ANY, _("Enlarge icons for high resolution display"));

  top_vsizer->Add(enlarge_icons_check_box_, wxSizerFlags().Left().Border(wxALL));

  SetSizerAndFit(top_vsizer);
}

void Global_ThemePage::LayoutField(wxSizer* top_vsizer, wxStaticText* label, wxComboBox* combo_box) {
  wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(label, wxSizerFlags(1).CenterVertical());
  hsizer->Add(combo_box, wxSizerFlags(1).CenterVertical().Border(wxLEFT));
  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
}

}  // namespace pref
}  // namespace jil
