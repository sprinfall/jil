#include "jil/pref/global_general_page.h"

#include "boost/algorithm/string.hpp"

#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"

#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"
#include "wx/statline.h"
#include "wx/stattext.h"
#include "wx/valnum.h"

#include "ui/static_box.h"

#include "editor/option.h"
#include "editor/util.h"

#include "jil/defs.h"
#include "jil/option.h"
#include "jil/util.h"

#include "jil/pref/common.h"

namespace jil {
namespace pref {

Global_GeneralPage::Global_GeneralPage(Options* options) : options_(options) {
  InitFileEncodingTable();
}

bool Global_GeneralPage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

bool Global_GeneralPage::TransferDataToWindow() {
  c_check_box_->SetValue((options_->cjk_filters & NS_FILTER_CHINESE) ==
                         NS_FILTER_CHINESE);
  j_check_box_->SetValue((options_->cjk_filters & NS_FILTER_JAPANESE) ==
                         NS_FILTER_JAPANESE);
  k_check_box_->SetValue((options_->cjk_filters & NS_FILTER_KOREAN) ==
                         NS_FILTER_KOREAN);

  int fenc_index = IndexFileEncoding(options_->file_encoding);
  if (fenc_index == wxNOT_FOUND) {
    fenc_index = 1;  // UTF-8
  }
  fenc_combo_box_->Select(fenc_index);

  switch_cwd_check_box_->SetValue(options_->switch_cwd);
  restore_files_check_box_->SetValue(options_->restore_files);
  show_path_check_box_->SetValue(options_->show_path);

  line_padding_spin_ctrl_->SetValue(options_->line_padding);

  return true;
}

bool Global_GeneralPage::TransferDataFromWindow() {
  UpdateFlag(options_->cjk_filters, NS_FILTER_CHINESE,
             c_check_box_->IsChecked());
  UpdateFlag(options_->cjk_filters, NS_FILTER_JAPANESE,
             j_check_box_->IsChecked());
  UpdateFlag(options_->cjk_filters, NS_FILTER_KOREAN,
             k_check_box_->IsChecked());

  options_->file_encoding = file_encodings_[fenc_combo_box_->GetSelection()];

  options_->switch_cwd = switch_cwd_check_box_->IsChecked();
  options_->restore_files = restore_files_check_box_->IsChecked();
  options_->show_path = show_path_check_box_->IsChecked();

  options_->line_padding = line_padding_spin_ctrl_->GetValue();

  return true;
}

void Global_GeneralPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  CreateEncodingSection(top_vsizer);

  switch_cwd_check_box_ = new wxCheckBox(this, wxID_ANY,
                                         _("Switch current working directory"));
  top_vsizer->Add(switch_cwd_check_box_, wxSizerFlags().Border(wxLTR));

  restore_files_check_box_ = new wxCheckBox(this, wxID_ANY,
                                            _("Restore last open files"));
  top_vsizer->Add(restore_files_check_box_, wxSizerFlags().Border(wxLTR));

  show_path_check_box_ = new wxCheckBox(this, wxID_ANY,
                                        _("Show file path in title bar"));
  top_vsizer->Add(show_path_check_box_, wxSizerFlags().Border(wxALL));

  CreateLinePaddingSection(top_vsizer);

  SetSizerAndFit(top_vsizer);
}

void Global_GeneralPage::CreateEncodingSection(wxSizer* top_vsizer) {
  ui::StaticBox* box = new ui::StaticBox(this, _("Encoding"));
  wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

  //--------------------------------------------------------------------------
  // CJK

  wxStaticText* cjk_label = new wxStaticText(box, wxID_ANY,
                                             _("Detect CJK encodings:"));

  // NOTE: Don't separate Chinese to Simplified and Traditional. Keep it simple.
  c_check_box_ = new wxCheckBox(box, wxID_ANY, _("Chinese"));
  j_check_box_ = new wxCheckBox(box, wxID_ANY, _("Japanese"));
  k_check_box_ = new wxCheckBox(box, wxID_ANY, _("Korean"));

  wxBoxSizer* cjk_vsizer = new wxBoxSizer(wxVERTICAL);
  cjk_vsizer->Add(cjk_label);

  {
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(c_check_box_);
    hsizer->Add(j_check_box_, wxSizerFlags().Border(wxLEFT));
    hsizer->Add(k_check_box_, wxSizerFlags().Border(wxLEFT));

    cjk_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxTOP));
  }

  box_vsizer->Add(cjk_vsizer, wxSizerFlags().Expand().Border(wxALL));

  //--------------------------------------------------------------------------
  // File Encoding

  wxStaticText* fenc_label = new wxStaticText(box, wxID_ANY,
                                              _("Default file encoding:"));

  fenc_combo_box_ = CreateReadonlyComboBox(box, wxID_ANY);
  fenc_combo_box_->SetMinSize(kMinComboBoxSize);
  InitFileEncodingComboBox(fenc_combo_box_);

  {
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(fenc_label, wxSizerFlags().Center());
    hsizer->AddStretchSpacer(1);
    hsizer->Add(fenc_combo_box_, wxSizerFlags().Center().Border(wxLEFT));

    box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  box->SetBodySizer(box_vsizer);
  top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));
}

void Global_GeneralPage::CreateLinePaddingSection(wxSizer* top_vsizer) {
  wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Line padding:"));

  line_padding_spin_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
  line_padding_spin_ctrl_->SetRange(kMinLinePadding, kMaxLinePadding);

  wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(label, wxSizerFlags().Center());
  hsizer->AddStretchSpacer(1);
  hsizer->Add(line_padding_spin_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
}

void Global_GeneralPage::InitFileEncodingTable() {
  using editor::GetEncodingById;

  file_encodings_.push_back(GetEncodingById(editor::ENCODING_ISO_8859_1));
  file_encodings_.push_back(GetEncodingById(editor::ENCODING_UTF8));
  file_encodings_.push_back(GetEncodingById(editor::ENCODING_UTF8_BOM));
  file_encodings_.push_back(GetEncodingById(editor::ENCODING_UTF16_BE));
  file_encodings_.push_back(GetEncodingById(editor::ENCODING_UTF16_LE));
  file_encodings_.push_back(GetEncodingById(editor::ENCODING_GB18030));
  file_encodings_.push_back(GetEncodingById(editor::ENCODING_BIG5));
}

void Global_GeneralPage::InitFileEncodingComboBox(wxComboBox* combo_box) {
  for (editor::Encoding& encoding : file_encodings_) {
    combo_box->Append(GetEncodingDisplayName(encoding.id));
  }
}

int Global_GeneralPage::IndexFileEncoding(editor::Encoding& encoding) const {
  for (size_t i = 0; i < file_encodings_.size(); ++i) {
    if (file_encodings_[i] == encoding) {
      return static_cast<int>(i);
    }
  }
  return wxNOT_FOUND;
}

}  // namespace pref
}  // namespace jil
