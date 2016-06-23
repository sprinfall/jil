#include "app/pref/global_font_page.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/fontutil.h"
#include "wx/listctrl.h"
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
#include "app/pref/font_list_ctrl.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(Global_FontPage, wxPanel)
EVT_LIST_ITEM_SELECTED(ID_FONT_LIST_CTRL, Global_FontPage::OnFontListSelectionChange)
EVT_LIST_ITEM_DESELECTED(ID_FONT_LIST_CTRL, Global_FontPage::OnFontListSelectionChange)
EVT_COMBOBOX(ID_FONT_NAME_COMBOBOX, Global_FontPage::OnNameComboBox)
EVT_COMBOBOX(ID_FONT_SIZE_COMBOBOX, Global_FontPage::OnSizeComboBox)
EVT_CHECKBOX(ID_FONT_FIXED_WIDTH_ONLY_CHECKBOX, Global_FontPage::OnFixedWidthOnlyCheckBox)
EVT_BUTTON(ID_FONT_RESET_BUTTON, Global_FontPage::OnResetButton)
END_EVENT_TABLE()

Global_FontPage::Global_FontPage(Options* options)
    : options_(options) {
  InitFonts();
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

#define FONT_LABEL_TEXT "Text Editor"
#define FONT_LABEL_LINE_NR "Line Number"
#define FONT_LABEL_TABS "Tabs"
#define FONT_LABEL_STATUS_BAR "Status Bar"

bool Global_FontPage::TransferDataToWindow() {
  // English labels.
  wxString labels[FONT_COUNT] = {
    wxT(FONT_LABEL_TEXT),
    wxT(FONT_LABEL_LINE_NR),
    wxT(FONT_LABEL_TABS),
    wxT(FONT_LABEL_STATUS_BAR),
  };

  // Localized labels.
  wxString tr_labels[FONT_COUNT] = {
    _(FONT_LABEL_TEXT),
    _(FONT_LABEL_LINE_NR),
    _(FONT_LABEL_TABS),
    _(FONT_LABEL_STATUS_BAR),
  };

  wxString label_sep = wxT(" | ");

  for (int i = 0; i < FONT_COUNT; ++i) {
    wxString label = labels[i];

    // Append localized label if it's different from the English one.
    if (tr_labels[i] != label) {
      label += label_sep + tr_labels[i];
    }

    font_list_ctrl_->InsertItem(i, label);

    //font_list_ctrl_->AddFont(fonts_[i], label);
  }

  //font_list_ctrl_->UpdateSizes();

  SetFontToWindow(wxNullFont);

  return true;
}

bool Global_FontPage::TransferDataFromWindow() {
  for (int i = 0; i < FONT_COUNT; ++i) {
    options_->fonts[i] = fonts_[i];
  }
  return true;
}

void Global_FontPage::InitFonts() {
  for (int i = 0; i < FONT_COUNT; ++i) {
    fonts_[i] = options_->fonts[i];
  }
}

void Global_FontPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  CreateTypeSection(top_vsizer);
  CreateFontSection(top_vsizer);

  wxButton* reset_button = new wxButton(this, ID_FONT_RESET_BUTTON, _("Reset All"));
  top_vsizer->Add(reset_button, wxSizerFlags().Right().Border(wxRIGHT));
  top_vsizer->AddSpacer(10);

  SetSizerAndFit(top_vsizer);
}

void Global_FontPage::CreateTypeSection(wxSizer* top_vsizer) {
  wxStaticText* label = new wxStaticText(this, wxID_ANY, _("GUI elements:"));

  //static const wxSize kFontListSize(-1, 150);
  //font_list_ctrl_ = new FontListCtrl;
  //font_list_ctrl_->Create(this, ID_FONT_LIST_CTRL/*, kFontListSize*/);

  long style = wxLC_LIST | wxLC_NO_HEADER | wxLC_SINGLE_SEL;
  font_list_ctrl_ = new wxListCtrl(this, ID_FONT_LIST_CTRL, wxDefaultPosition, wxDefaultSize, style);
  font_list_ctrl_->AppendColumn(wxEmptyString);

  top_vsizer->Add(label, wxSizerFlags().Left().Border(wxLEFT|wxTOP));
  top_vsizer->Add(font_list_ctrl_, wxSizerFlags(1).Expand().Border(wxALL));
}

void Global_FontPage::CreateFontSection(wxSizer* top_vsizer) {
  bool fixed_width_only = false;

  name_combo_box_ = new wxComboBox(this, ID_FONT_NAME_COMBOBOX);
  InitNameComboBox(name_combo_box_, fixed_width_only);  // TODO: Slow to list fonts.

  size_combo_box_ = new wxComboBox(this, ID_FONT_SIZE_COMBOBOX, wxEmptyString, wxDefaultPosition, kNumTextSize);
  InitSizeComboBox(size_combo_box_);

  fixed_width_check_box_ = new wxCheckBox(this, ID_FONT_FIXED_WIDTH_ONLY_CHECKBOX, _("Fixed width only"));
  fixed_width_check_box_->SetValue(fixed_width_only);

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  // Size proportion: 3 : 1
  hsizer->Add(name_combo_box_, wxSizerFlags(3));
  hsizer->Add(size_combo_box_, wxSizerFlags(1).Border(wxLEFT, 10));
  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));

  top_vsizer->Add(fixed_width_check_box_, wxSizerFlags().Expand().Border(wxALL));
}

// TODO
FontType Global_FontPage::GetSelectedFontType() const {
  //int index = font_list_ctrl_->selected_index();
  //if (index >= 0 && index < FONT_COUNT) {
  //  return static_cast<FontType>(index);
  //}
  return FONT_COUNT;
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
  if (font.IsOk()) {
    wxString face_name = font.GetFaceName();
    name_combo_box_->SetValue(face_name);

    wxString size_str = wxString::Format(wxT("%d"), font.GetPointSize());
    size_combo_box_->SetValue(size_str);

    name_combo_box_->Enable(true);
    size_combo_box_->Enable(true);
  } else {
    name_combo_box_->SetValue(wxEmptyString);
    size_combo_box_->SetValue(wxEmptyString);

    name_combo_box_->Enable(false);
    size_combo_box_->Enable(false);
  }
}

void Global_FontPage::OnFontListSelectionChange(wxListEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type != FONT_COUNT) {
    SetFontToWindow(fonts_[font_type]);
  } else {
    SetFontToWindow(wxNullFont);
  }
}

void Global_FontPage::OnNameComboBox(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type == FONT_COUNT) {
    return;
  }

  //wxFont& font = fonts_[font_type];

  //wxString name = name_combo_box_->GetValue();
  //if (!font.SetFaceName(name)) {
  //  return;
  //}

  //font_list_ctrl_->SetFont(font_type, font);
}

void Global_FontPage::OnSizeComboBox(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type == FONT_COUNT) {
    return;
  }

  wxFont& font = fonts_[font_type];

  long size = GetDefaultFontSize();
  if (!size_combo_box_->GetValue().ToLong(&size)) {
    return;
  }
  if (size < kMinFontSize || size > kMaxFontSize) {
    size = GetDefaultFontSize();
  }
  font.SetPointSize(static_cast<int>(size));

  //font_list_ctrl_->SetFont(font_type, font);
}

void Global_FontPage::OnFixedWidthOnlyCheckBox(wxCommandEvent& evt) {
  wxString value = name_combo_box_->GetValue();

  name_combo_box_->Clear();
  InitNameComboBox(name_combo_box_, evt.IsChecked());

  name_combo_box_->SetValue(value);
}

void Global_FontPage::OnResetButton(wxCommandEvent& evt) {
  InitFonts();

  // Update font list ctrl.
  //for (int i = 0; i < FONT_COUNT; ++i) {
  //  font_list_ctrl_->SetFont(i, fonts_[i]);
  //}
  //font_list_ctrl_->UpdateSizes();

  // Update font name and size combo boxes.
  FontType font_type = GetSelectedFontType();
  if (font_type != FONT_COUNT) {
    SetFontToWindow(fonts_[font_type]);
  }
}

}  // namespace pref
}  // namespace jil
