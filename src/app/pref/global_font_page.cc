#include "app/pref/global_font_page.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"

#include "editor/defs.h"
#include "editor/option.h"
#include "editor/util.h"

#include "app/font_util.h"
#include "app/option.h"
#include "app/util.h"

#include "app/pref/common.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(Global_FontPage, wxPanel)
EVT_COMBOBOX(ID_FONT_TYPE_COMBOBOX, Global_FontPage::OnTypeComboBox)
EVT_COMBOBOX(ID_FONT_NAME_COMBOBOX, Global_FontPage::OnNameComboBox)
EVT_COMBOBOX(ID_FONT_SIZE_COMBOBOX, Global_FontPage::OnSizeComboBox)
EVT_CHECKBOX(ID_FONT_FIXED_WIDTH_ONLY_CHECKBOX, Global_FontPage::OnFixedWidthOnlyCheckBox)
EVT_BUTTON(ID_FONT_RESET_BUTTON, Global_FontPage::OnResetButton)
END_EVENT_TABLE()

Global_FontPage::Global_FontPage(Options* options)
    : options_(options) {
  for (int i = 0; i < FONT_COUNT; ++i) {
    fonts_[i] = options_->fonts[i];
  }
}

Global_FontPage::~Global_FontPage() {
}

bool Global_FontPage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

bool Global_FontPage::TransferDataToWindow() {
  TransferFontToWindow();
  return true;
}

bool Global_FontPage::TransferDataFromWindow() {
  for (int i = 0; i < FONT_COUNT; ++i) {
    options_->fonts[i] = fonts_[i];
  }
  return true;
}

void Global_FontPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  CreateTypeSection(top_vsizer);
  CreateFontSection(top_vsizer);

  wxButton* reset_button = new wxButton(this, ID_FONT_RESET_BUTTON, wxT("Reset"));
  top_vsizer->Add(reset_button, wxSizerFlags().Right().Border(wxRIGHT));

  SetSizerAndFit(top_vsizer);
}

void Global_FontPage::CreateTypeSection(wxSizer* top_vsizer) {
  wxString font_types[FONT_COUNT] = {
    _("Text Editor"),
    _("Line Number"),
    _("Tabs"),
    _("Status Bar"),
  };

  wxStaticText* type_label = new wxStaticText(this, wxID_ANY, wxT("Font type:"));

  type_combo_box_ = CreateReadonlyComboBox(this, ID_FONT_TYPE_COMBOBOX);
  type_combo_box_->SetMinSize(kMinComboBoxSize);
  type_combo_box_->Append(FONT_COUNT, font_types);
  type_combo_box_->Select(FONT_TEXT);

  wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(type_label, wxSizerFlags().Center());
  hsizer->AddStretchSpacer(1);  // TODO
  hsizer->Add(type_combo_box_, wxSizerFlags().Center().Border(wxLEFT));
  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
}

void Global_FontPage::CreateFontSection(wxSizer* top_vsizer) {
  bool fixed_width_only = false;

  wxStaticText* name_label = new wxStaticText(this, wxID_ANY, wxT("Name:"));
  name_combo_box_ = new wxComboBox(this, ID_FONT_NAME_COMBOBOX/*, wxEmptyString, wxDefaultPosition, kStrTextSize*/);
  InitNameComboBox(name_combo_box_, fixed_width_only);

  wxStaticText* size_label = new wxStaticText(this, wxID_ANY, wxT("Size:"));
  size_combo_box_ = new wxComboBox(this, ID_FONT_SIZE_COMBOBOX, wxEmptyString, wxDefaultPosition, kNumTextSize);
  InitSizeComboBox(size_combo_box_);

  fixed_width_check_box_ = new wxCheckBox(this, ID_FONT_FIXED_WIDTH_ONLY_CHECKBOX, wxT("Fixed width only"));
  fixed_width_check_box_->SetValue(fixed_width_only);

  wxBoxSizer* name_vsizer = new wxBoxSizer(wxVERTICAL);
  name_vsizer->Add(name_label);
  name_vsizer->Add(name_combo_box_, wxSizerFlags().Border(wxTOP, 2));

  wxBoxSizer* size_vsizer = new wxBoxSizer(wxVERTICAL);
  size_vsizer->Add(size_label);
  size_vsizer->Add(size_combo_box_, wxSizerFlags().Border(wxTOP, 2));

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(name_vsizer, wxSizerFlags(3));
  hsizer->Add(size_vsizer, wxSizerFlags(1).Border(wxLEFT));
  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));

  top_vsizer->Add(fixed_width_check_box_, wxSizerFlags().Expand().Border(wxALL));
}

void Global_FontPage::InitNameComboBox(wxComboBox* combo_box, bool fixed_width_only) {
  OrderedFontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, fixed_width_only);
  std::set<wxString>::iterator it = fe.facenames.begin();
  for (; it != fe.facenames.end(); ++it) {
    combo_box->Append(*it);
  }
}

void Global_FontPage::InitSizeComboBox(wxComboBox* combo_box) {
  for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
    combo_box->Append(wxString::Format(wxT("%d"), i));
  }
}

void Global_FontPage::SetFontToWindow(const wxFont& font) {
  wxString face_name = font.GetFaceName();
  name_combo_box_->SetValue(face_name);

  wxString size_str = wxString::Format(wxT("%d"), font.GetPointSize());
  size_combo_box_->SetValue(size_str);
}

void Global_FontPage::TransferFontToWindow() {
  SetFontToWindow(fonts_[type_combo_box_->GetSelection()]);
}

bool Global_FontPage::TransferFontFromWindow() {
  wxFont& font = fonts_[type_combo_box_->GetSelection()];

  wxString name = name_combo_box_->GetValue();
  if (!font.SetFaceName(name)) {
    return false;
  }

  long size = kDefaultFontSize;
  if (!size_combo_box_->GetValue().ToLong(&size)) {
    return false;
  }
  if (size < kMinFontSize || size > kMaxFontSize) {
    size = kDefaultFontSize;
  }
  font.SetPointSize(static_cast<int>(size));

  return true;
}

void Global_FontPage::OnTypeComboBox(wxCommandEvent& evt) {
  TransferFontToWindow();
}

void Global_FontPage::OnNameComboBox(wxCommandEvent& evt) {
  TransferFontFromWindow();
}

void Global_FontPage::OnSizeComboBox(wxCommandEvent& evt) {
  TransferFontFromWindow();
}

void Global_FontPage::OnFixedWidthOnlyCheckBox(wxCommandEvent& evt) {
  wxString value = name_combo_box_->GetValue();

  name_combo_box_->Clear();
  InitNameComboBox(name_combo_box_, evt.IsChecked());

  name_combo_box_->SetValue(value);
}

void Global_FontPage::OnResetButton(wxCommandEvent& evt) {
  int type = type_combo_box_->GetSelection();
  fonts_[type] = options_->fonts[type];
  SetFontToWindow(fonts_[type]);
}

}  // namespace pref
}  // namespace jil
