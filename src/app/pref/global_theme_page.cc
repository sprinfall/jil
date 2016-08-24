#include "app/pref/global_theme_page.h"

#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"

#include "editor/option.h"
#include "editor/util.h"

#include "app/app.h"
#include "app/defs.h"
#include "app/option.h"
#include "app/util.h"

#include "app/pref/common.h"

#define kTrIconLow      _("Low")
#define kTrIconMedium   _("Medium")
#define kTrIconHigh     _("High")

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
  icon_combo_box_->Select(options_->icon_resolution);
  return true;
}

bool Global_ThemePage::TransferDataFromWindow() {
  options_->theme = theme_combo_box_->GetValue();
  options_->icon_resolution = static_cast<Resolution>(icon_combo_box_->GetSelection());
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

  wxStaticText* icon_label = new wxStaticText(this, wxID_ANY, _("Resolution of icons:"));
  icon_combo_box_ = CreateReadonlyComboBox(this, wxID_ANY);

  icon_combo_box_->Append(kTrIconLow);
  icon_combo_box_->Append(kTrIconMedium);
  icon_combo_box_->Append(kTrIconHigh);

  LayoutField(top_vsizer, icon_label, icon_combo_box_);

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
