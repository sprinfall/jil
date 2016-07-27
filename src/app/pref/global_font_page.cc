#include "app/pref/global_font_page.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/dc.h"
#include "wx/fontutil.h"
#include "wx/listctrl.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"

#include "editor/defs.h"
#include "editor/option.h"
#include "editor/util.h"

#include "ui/string_list_ctrl.h"

#include "app/font_util.h"
#include "app/option.h"
#include "app/util.h"
#include "app/pref/common.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(Global_FontPage, wxPanel)
EVT_LIST_ITEM_SELECTED(ID_FONT_LIST_CTRL, Global_FontPage::OnFontListSelectionChange)
EVT_LIST_ITEM_DESELECTED(ID_FONT_LIST_CTRL, Global_FontPage::OnFontListSelectionChange)
EVT_COMBOBOX(ID_FONT_NAME_COMBOBOX, Global_FontPage::OnNameComboBox)
EVT_COMBOBOX(ID_FONT_SIZE_COMBOBOX, Global_FontPage::OnSizeComboBox)
EVT_BUTTON(ID_FONT_USE_DEFAULT_BUTTON, Global_FontPage::OnUseDefaultButton)
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

bool Global_FontPage::TransferDataToWindow() {
  SetFontToWindow(wxNullFont);
  use_default_button_->Enable(false);
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

  use_default_button_ = new wxButton(this, ID_FONT_USE_DEFAULT_BUTTON, _("Use default font"));
  top_vsizer->Add(use_default_button_, wxSizerFlags().Right().Border(wxRIGHT));
  top_vsizer->AddSpacer(10);

  SetSizerAndFit(top_vsizer);
}

void Global_FontPage::CreateTypeSection(wxSizer* top_vsizer) {
  wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Display items:"));

  font_list_ctrl_ = new ui::StringListCtrl;
  font_list_ctrl_->Create(this, ID_FONT_LIST_CTRL);

  wxString tr_labels[FONT_COUNT] = {
    _("Text Editor"),
    _("Line Number"),
    _("Tabs"),
    _("Status Bar"),
  };

  for (int i = 0; i < FONT_COUNT; ++i) {
    font_list_ctrl_->AppendString(tr_labels[i]);
  }

  font_list_ctrl_->UpdateSizes();

  top_vsizer->Add(label, wxSizerFlags().Left().Border(wxLEFT|wxTOP));
  top_vsizer->Add(font_list_ctrl_, wxSizerFlags().Expand().Border(wxALL));
}

void Global_FontPage::CreateFontSection(wxSizer* top_vsizer) {
  name_combo_box_ = new wxComboBox(this, ID_FONT_NAME_COMBOBOX);
  InitNameComboBox(name_combo_box_);

  size_combo_box_ = new wxComboBox(this,
                                   ID_FONT_SIZE_COMBOBOX,
                                   wxEmptyString,
                                   wxDefaultPosition,
                                   kSmallNumTextSize);
  InitSizeComboBox(size_combo_box_);

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(name_combo_box_, wxSizerFlags(1));
  hsizer->Add(size_combo_box_, wxSizerFlags().Border(wxLEFT, 10));
  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
}

FontType Global_FontPage::GetSelectedFontType() const {
  int index = font_list_ctrl_->selected_index();
  if (index >= 0 && index < FONT_COUNT) {
    return static_cast<FontType>(index);
  }
  return FONT_COUNT;
}

// TODO: Slow to list fonts.
void Global_FontPage::InitNameComboBox(wxComboBox* combo_box) {
  OrderedFontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, false);

  for (const wxString& facename : fe.facenames) {
    combo_box->Append(facename);
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
    use_default_button_->Enable(true);
  } else {
    SetFontToWindow(wxNullFont);
    use_default_button_->Enable(false);
  }
}

void Global_FontPage::OnNameComboBox(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type == FONT_COUNT) {
    return;
  }

  wxFont& font = fonts_[font_type];

  wxString name = name_combo_box_->GetValue();
  if (!font.SetFaceName(name)) {
    return;
  }
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
}

void Global_FontPage::OnUseDefaultButton(wxCommandEvent& evt) {
  FontType font_type = GetSelectedFontType();
  if (font_type != FONT_COUNT) {
    fonts_[font_type] = GetDefaultFont(font_type);

    // Update font name and size combo boxes.
    SetFontToWindow(fonts_[font_type]);
  }
}

}  // namespace pref
}  // namespace jil
