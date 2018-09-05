#include "jil/pref/editor_general_page.h"

#include <algorithm>  // sort()

#include "wx/checkbox.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"

#include "ui/static_box.h"

#include "editor/option.h"

#include "jil/pref/common.h"

namespace jil {
namespace pref {

Editor_GeneralPage::Editor_GeneralPage(editor::Options* options)
  : options_(options) {
}

Editor_GeneralPage::~Editor_GeneralPage() {
}

bool Editor_GeneralPage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

wxString Editor_GeneralPage::JoinRulers(const std::vector<int>& rulers) {
  wxString rulers_str;
  for (int ruler : rulers) {
    if (!rulers_str.IsEmpty()) {
      rulers_str += wxT(",");
    }
    rulers_str += wxString::Format(wxT("%d"), ruler);
  }
  return rulers_str;
}

void Editor_GeneralPage::SplitRulers(const wxString& rulers_str,
                                     std::vector<int>& rulers) {
  wxArrayString ruler_strs = wxSplit(rulers_str, wxT(','));
  long ruler = 0;
  for (std::size_t i = 0; i < ruler_strs.size(); ++i) {
    if (ruler_strs[i].ToLong(&ruler)) {
      if (ruler > 0) {
        rulers.push_back(static_cast<int>(ruler));
      }
    }
  }
}

bool Editor_GeneralPage::TransferDataToWindow() {
  show_hscrollbar_check_box_->SetValue(options_->view.show_hscrollbar);
  show_number_check_box_->SetValue(options_->view.show_number);
  show_space_check_box_->SetValue(options_->view.show_space);
  wrap_check_box_->SetValue(options_->view.wrap);

  rulers_text_ctrl_->SetValue(JoinRulers(options_->view.rulers));

  wxString delimiters(options_->text.delimiters);
  delimiters_text_ctrl_->SetValue(delimiters);

  return true;
}

bool Editor_GeneralPage::TransferDataFromWindow() {
  options_->view.show_hscrollbar = show_hscrollbar_check_box_->GetValue();
  options_->view.show_number = show_number_check_box_->GetValue();
  options_->view.show_space = show_space_check_box_->GetValue();
  options_->view.wrap = wrap_check_box_->GetValue();

  options_->view.rulers.clear();
  SplitRulers(rulers_text_ctrl_->GetValue(), options_->view.rulers);

  std::wstring delimiters = delimiters_text_ctrl_->GetValue().ToStdWstring();

  // Remove ' ' and '\t' and duplicate elements.
  std::sort(delimiters.begin(), delimiters.end());  // std::unique needs this.
  auto end = std::remove(delimiters.begin(), delimiters.end(), L' ');
  end = std::remove(delimiters.begin(), end, L'\t');
  end = std::unique(delimiters.begin(), end);
  if (end != delimiters.end()) {
    delimiters.erase(end, delimiters.end());
  }

  options_->text.delimiters = delimiters;

  return true;
}

void Editor_GeneralPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  CreateDisplaySection(top_vsizer);
  CreateRulersSection(top_vsizer);
  CreateDelimitersSection(top_vsizer);

  SetSizerAndFit(top_vsizer);
}

void Editor_GeneralPage::CreateDisplaySection(wxSizer* top_vsizer) {
  ui::StaticBox* box = new ui::StaticBox(this, _("Display"));
  wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

  show_hscrollbar_check_box_ = new wxCheckBox(box, wxID_ANY,
                                              _("Show horizontal scrollbar"));
  box_vsizer->Add(show_hscrollbar_check_box_);

  show_number_check_box_ = new wxCheckBox(box, wxID_ANY,
                                          _("Show line numbers"));
  box_vsizer->Add(show_number_check_box_, wxSizerFlags().Border(wxTOP));

  show_space_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show white spaces"));
  box_vsizer->Add(show_space_check_box_, wxSizerFlags().Border(wxTOP));

  wrap_check_box_ = new wxCheckBox(box, wxID_ANY, _("Wrap line"));
  box_vsizer->Add(wrap_check_box_, wxSizerFlags().Border(wxTOP));

  box->SetBodySizer(box_vsizer);
  top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));
}

void Editor_GeneralPage::CreateRulersSection(wxSizer* top_vsizer) {
  wxStaticText* rulers_label = new wxStaticText(this, wxID_ANY, _("Rulers:"));
  rulers_text_ctrl_ = CreateTextCtrl(this, wxID_ANY);

  wxSizer* rulers_hsizer = new wxBoxSizer(wxHORIZONTAL);
  rulers_hsizer->Add(rulers_label, wxSizerFlags().Center());
  rulers_hsizer->AddStretchSpacer(1);
  rulers_hsizer->Add(rulers_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

  top_vsizer->Add(rulers_hsizer, wxSizerFlags().Expand().Border(wxALL));
}

void Editor_GeneralPage::CreateDelimitersSection(wxSizer* top_vsizer) {
  wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Delimiters:"));
  delimiters_text_ctrl_ = CreateTextCtrl(this, wxID_ANY, kStrTextSize);
  delimiters_text_ctrl_->SetToolTip(
      _("Word delimiters (excluding ' ' and '\t')."));

  wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(label, wxSizerFlags().Center());
  hsizer->Add(delimiters_text_ctrl_, wxSizerFlags(1).Center().Border(wxLEFT));

  top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
}

}  // namespace pref
}  // namespace jil
