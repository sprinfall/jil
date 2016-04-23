#include "app/pref/editor_indent_page.h"

#include "boost/algorithm/string.hpp"

#include "wx/checkbox.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "wx/valnum.h"

#include "ui/static_box.h"

#include "editor/option.h"

#include "app/defs.h"
#include "app/pref/option_list_ctrl.h"
#include "app/pref/common.h"

namespace jil {
namespace pref {

static const wxSize kOptionListSize(-1, 120);

Editor_IndentPage::Editor_IndentPage(editor::Options* options)
  : options_(options) {
}

Editor_IndentPage::~Editor_IndentPage() {
}

bool Editor_IndentPage::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  CreateControls();

  return true;
}

bool Editor_IndentPage::TransferDataToWindow() {
  tab_stop_text_ctrl_->SetValue(IntToStr(options_->text.tab_stop));
  expand_tab_check_box_->SetValue(options_->text.expand_tab);
  guess_check_box_->SetValue(options_->text.guess_tab);

  std::wstring indent_keys = boost::join(options_->text.indent_keys, L",");
  indent_keys_text_ctrl_->SetValue(indent_keys);

  option_list_ctrl_->StartBatch();
  editor::OptionTable& indent_options = options_->text.indent_options;
  for (size_t i = 0; i < indent_options.size(); ++i) {
    option_list_ctrl_->AddOption(indent_options[i]);
  }
  option_list_ctrl_->EndBatch();

  return true;
}

bool Editor_IndentPage::TransferDataFromWindow() {
  int tab_stop = StrToInt(tab_stop_text_ctrl_->GetValue());
  options_->text.tab_stop = ValidateInt(tab_stop, editor::kMinTabStop, editor::kMaxTabStop);

  options_->text.expand_tab = expand_tab_check_box_->GetValue();
  options_->text.guess_tab = guess_check_box_->GetValue();

  std::wstring indent_keys_str = indent_keys_text_ctrl_->GetValue().ToStdWstring();
  // Split
  std::vector<std::wstring> indent_keys;
  boost::split(indent_keys, indent_keys_str, boost::is_any_of(L", "), boost::token_compress_on);
  // Remove empty strings.
  std::vector<std::wstring>::iterator end = std::remove(indent_keys.begin(), indent_keys.end(), L"");
  if (end != indent_keys.end()) {
    indent_keys.erase(end, indent_keys.end());
  }
  options_->text.indent_keys = indent_keys;

  options_->text.indent_options.clear();
  int indent_options_count = option_list_ctrl_->GetCount();
  for (int i = 0; i < indent_options_count; ++i) {
    options_->text.indent_options.push_back(*option_list_ctrl_->GetOption(i));
  }

  return true;
}

void Editor_IndentPage::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  CreateTabsSection(top_vsizer);

  // Indent keys
  wxStaticText* indent_keys_label = new wxStaticText(this, wxID_ANY, _("Indent keys:"));
  indent_keys_text_ctrl_ = CreateTextCtrl(this, wxID_ANY, kStrTextSize);
  {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(indent_keys_label, wxSizerFlags().Center());
    hsizer->Add(indent_keys_text_ctrl_, wxSizerFlags(1).Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  CreateDynamicSection(top_vsizer);

  SetSizerAndFit(top_vsizer);
}

void Editor_IndentPage::CreateTabsSection(wxSizer* top_vsizer) {
  ui::StaticBox* box = new ui::StaticBox(this, _("Tabs"));
  wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText* tab_stop_label = new wxStaticText(box, wxID_ANY, _("Tab stop:"));

  wxIntegerValidator<int> validator(NULL, 0);
  validator.SetMin(editor::kMinTabStop);
  validator.SetMax(editor::kMaxTabStop);

  tab_stop_text_ctrl_ = CreateTextCtrl(box, ID_TAB_STOP_TEXTCTRL, kNumTextSize, validator);

  expand_tab_check_box_ = new wxCheckBox(box, ID_EXPAND_TAB_CHECKBOX, _("Expand tabs"));

  guess_check_box_ = new wxCheckBox(box, ID_GUESS_CHECKBOX, _("Guess from existing lines"));

  {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(tab_stop_label, wxSizerFlags().Center());
    hsizer->Add(tab_stop_text_ctrl_, wxSizerFlags().Center().Border(wxLR));
    hsizer->AddStretchSpacer(1);
    hsizer->Add(expand_tab_check_box_, wxSizerFlags().Center().Border(wxLEFT));

    box_vsizer->Add(hsizer, wxSizerFlags().Expand());
  }

  box_vsizer->Add(guess_check_box_, wxSizerFlags().Expand().Border(wxTOP));

  box->SetBodySizer(box_vsizer);
  top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));
}

void Editor_IndentPage::CreateDynamicSection(wxSizer* top_vsizer) {
  top_vsizer->AddSpacer(10);

  wxSizer* sep_hsizer = CreateSeparator(this, _("Dynamic options"), true);
  top_vsizer->Add(sep_hsizer, wxSizerFlags().Expand().Border(wxALL));

  option_list_ctrl_ = new OptionListCtrl();
  option_list_ctrl_->Create(this, wxID_ANY, kOptionListSize, false);

  top_vsizer->Add(option_list_ctrl_, wxSizerFlags().Expand().Border(wxALL));
}

}  // namespace pref
}  // namespace jil
