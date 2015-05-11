#include "app/pref/general_config_panel.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/stattext.h"

#include "editor/defs.h"

#include "app/defs.h"
#include "app/option.h"
#include "app/pref/defs.h"

namespace jil {
namespace pref {

GeneralConfigPanel::GeneralConfigPanel(Options* options)
    : options_(options) {
}

GeneralConfigPanel::~GeneralConfigPanel() {
}

bool GeneralConfigPanel::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

void GeneralConfigPanel::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  //----------------------------------------------------------------------------
  // Encoding

  {
    wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Encoding"));
    wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

    //--------------------------------------------------------------------------
    // CJK

    {
      wxStaticText* cjk_label = new wxStaticText(box, wxID_ANY, _("Detect CJK encodings:"));

      // NOTE:
      // Don't separate Chinese to Simplified and Traditional. Keep it simple.
      c_check_box_ = new wxCheckBox(box, wxID_ANY, _("Chinese"));
      j_check_box_ = new wxCheckBox(box, wxID_ANY, _("Japanese"));
      k_check_box_ = new wxCheckBox(box, wxID_ANY, _("Korean"));

      wxBoxSizer* cjk_vsizer = new wxBoxSizer(wxVERTICAL);
      cjk_vsizer->Add(cjk_label);

      wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(c_check_box_);
      hsizer->Add(j_check_box_, wxSizerFlags().Border(wxLEFT));
      hsizer->Add(k_check_box_, wxSizerFlags().Border(wxLEFT));
      cjk_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxTOP));

      box_vsizer->Add(cjk_vsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    //--------------------------------------------------------------------------
    // File Encoding

    {
      wxStaticText* label = new wxStaticText(box, wxID_ANY, _("Default file encoding:"));

      fenc_combo_box_ = new wxComboBox(box, wxID_ANY, wxEmptyString);
      fenc_combo_box_->SetMinSize(kMinComboBoxSize);
      InitFileEncodingComboBox(fenc_combo_box_);

      wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(label, wxSizerFlags().Center());
      hsizer->AddStretchSpacer(1);
      hsizer->Add(fenc_combo_box_, wxSizerFlags().Center().Border(wxLEFT));

      box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //----------------------------------------------------------------------------

  {
    switch_cwd_check_box_ = new wxCheckBox(this, wxID_ANY, _("Switch current working directory"));
    top_vsizer->Add(switch_cwd_check_box_, wxSizerFlags().Border(wxLTR));
  }

  {
    restore_files_check_box_ = new wxCheckBox(this, wxID_ANY, _("Restore last open files"));
    top_vsizer->Add(restore_files_check_box_, wxSizerFlags().Border(wxLTR));
  }

  {
    show_path_check_box_ = new wxCheckBox(this, wxID_ANY, _("Show file path in title bar"));
    top_vsizer->Add(show_path_check_box_, wxSizerFlags().Border(wxALL));
  }

  //----------------------------------------------------------------------------
  // Line padding

  {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Line padding:"));
    wxTextCtrl* text_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->Add(text_ctrl, wxSizerFlags().Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //----------------------------------------------------------------------------

  fonts_button_ = new wxButton(this, wxID_ANY, _("&Fonts..."));

  top_vsizer->Add(fonts_button_, wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxALL));

  //----------------------------------------------------------------------------

  SetSizerAndFit(top_vsizer);
}

bool GeneralConfigPanel::TransferDataToWindow() {
  switch_cwd_check_box_->SetValue(options_->switch_cwd);
  restore_files_check_box_->SetValue(options_->restore_files);
  show_path_check_box_->SetValue(options_->show_path);

  return true;
}

bool GeneralConfigPanel::TransferDataFromWindow() {
  //wxCommandEvent dummy;
  //ChangedUseMarkdown(dummy);
  //ChangedSpellcheck(dummy);
  return true;
}

void GeneralConfigPanel::InitFileEncodingComboBox(wxComboBox* combo_box) {
  combo_box->Append(ENCODING_DISPLAY_NAME_ISO_8859_1);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF8);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF8_BOM);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF16_BE);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF16_LE);
  combo_box->Append(ENCODING_DISPLAY_NAME_GB18030);
  combo_box->Append(ENCODING_DISPLAY_NAME_BIG5);
}

}  // namespace pref
}  // namespace jil