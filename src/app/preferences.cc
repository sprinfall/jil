#include "app/preferences.h"

#include <vector>
#include "boost/algorithm/string.hpp"

#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/notebook.h"
#include "wx/panel.h"
#include "wx/scrolwin.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"
#include "wx/statline.h"
#include "wx/stattext.h"
#include "wx/valnum.h"

#include "ui/static_box.h"

#include "editor/defs.h"
#include "editor/option.h"
#include "editor/util.h"

#include "app/defs.h"
#include "app/font_util.h"
#include "app/option.h"
#include "app/option_list_ctrl.h"
#include "app/util.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

static const wxSize kMinComboBoxSize(120, -1);
static const wxSize kNumTextSize(60, -1);
static const wxSize kStrTextSize(180, -1);

// Add or remove a flag from the flags.
static void UpdateFlag(int& flags, int flag, bool enable) {
  if (enable) {
    flags |= flag;
  } else {
    flags &= ~flag;
  }
}

static wxString IntToStr(int i) {
  return wxString::Format(wxT("%d"), i);
}

static int StrToInt(const wxString& str) {
  long i = 0;
  str.ToLong(&i);
  return static_cast<int>(i);
}

static int ValidateInt(int i, int min, int max) {
  if (i < min) {
    return min;
  }
  if (i > max) {
    return max;
  }
  return i;
}

static wxStaticText* CreateStaticText(wxWindow* parent, const wxString& label, bool bold = false) {
  wxStaticText* static_text = new wxStaticText(parent, wxID_ANY, label);
  if (bold) {
    wxFont font = static_text->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    static_text->SetFont(font);
  }
  return static_text;
}

static wxSizer* CreateSeparator(wxWindow* parent, const wxString& label, bool bold = false) {
  wxStaticText* static_text = CreateStaticText(parent, label, bold);
  wxStaticLine* sep = new wxStaticLine(parent);

  wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(static_text, wxSizerFlags().Center());
  hsizer->Add(sep, wxSizerFlags(1).Center().Border(wxLEFT));
  return hsizer;
}

static inline wxComboBox* CreateReadonlyComboBox(wxWindow* parent, wxWindowID id) {
  return new wxComboBox(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
}

static inline wxTextCtrl* CreateTextCtrl(wxWindow* parent,
                                         wxWindowID id,
                                         const wxSize& size = wxDefaultSize,
                                         const wxValidator& validator = wxDefaultValidator) {
  return new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, size, 0, validator);
}

namespace pref {

////////////////////////////////////////////////////////////////////////////////

enum Id {
  ID_PREF_BEGIN = wxID_HIGHEST + 100,

  ID_FONT_TYPE_COMBOBOX,
  ID_FONT_NAME_COMBOBOX,
  ID_FONT_SIZE_COMBOBOX,
  ID_FONT_FIXED_WIDTH_ONLY_CHECKBOX,
  ID_FONT_RESET_BUTTON,

  ID_TAB_STOP_TEXTCTRL,
  ID_EXPAND_TAB_CHECKBOX,
  ID_GUESS_CHECKBOX,
};

////////////////////////////////////////////////////////////////////////////////

class Global_GeneralPage : public wxPanel {
public:
  Global_GeneralPage(Options* options) : options_(options) {
    InitFileEncodingTable();
  }

  virtual ~Global_GeneralPage() {
  }

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY) {
    if (!wxPanel::Create(parent, id)) {
      return false;
    }

    CreateControls();

    return true;
  }

  virtual bool TransferDataToWindow() override {
    c_check_box_->SetValue((options_->cjk_filters & NS_FILTER_CHINESE) == NS_FILTER_CHINESE);
    j_check_box_->SetValue((options_->cjk_filters & NS_FILTER_JAPANESE) == NS_FILTER_JAPANESE);
    k_check_box_->SetValue((options_->cjk_filters & NS_FILTER_KOREAN) == NS_FILTER_KOREAN);

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

  virtual bool TransferDataFromWindow() override {
    UpdateFlag(options_->cjk_filters, NS_FILTER_CHINESE, c_check_box_->IsChecked());
    UpdateFlag(options_->cjk_filters, NS_FILTER_JAPANESE, j_check_box_->IsChecked());
    UpdateFlag(options_->cjk_filters, NS_FILTER_KOREAN, k_check_box_->IsChecked());

    options_->file_encoding = file_encodings_[fenc_combo_box_->GetSelection()];

    options_->switch_cwd = switch_cwd_check_box_->IsChecked();
    options_->restore_files = restore_files_check_box_->IsChecked();
    options_->show_path = show_path_check_box_->IsChecked();

    options_->line_padding = line_padding_spin_ctrl_->GetValue();

    return true;
  }

private:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    CreateEncodingSection(top_vsizer);

    switch_cwd_check_box_ = new wxCheckBox(this, wxID_ANY, _("Switch current working directory"));
    top_vsizer->Add(switch_cwd_check_box_, wxSizerFlags().Border(wxLTR));

    restore_files_check_box_ = new wxCheckBox(this, wxID_ANY, _("Restore last open files"));
    top_vsizer->Add(restore_files_check_box_, wxSizerFlags().Border(wxLTR));

    show_path_check_box_ = new wxCheckBox(this, wxID_ANY, _("Show file path in title bar"));
    top_vsizer->Add(show_path_check_box_, wxSizerFlags().Border(wxALL));

    CreateLinePaddingSection(top_vsizer);

    SetSizerAndFit(top_vsizer);
  }

  void CreateEncodingSection(wxSizer* top_vsizer) {
    ui::StaticBox* box = new ui::StaticBox(this, _("Encoding"));
    wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

    //--------------------------------------------------------------------------
    // CJK

    wxStaticText* cjk_label = new wxStaticText(box, wxID_ANY, _("Detect CJK encodings:"));

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

    wxStaticText* fenc_label = new wxStaticText(box, wxID_ANY, _("Default file encoding:"));

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

  void CreateLinePaddingSection(wxSizer* top_vsizer) {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Line padding:"));

    line_padding_spin_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
    line_padding_spin_ctrl_->SetRange(kMinLinePadding, kMaxLinePadding);

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->AddStretchSpacer(1);
    hsizer->Add(line_padding_spin_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  void InitFileEncodingTable() {
    using namespace editor;

    file_encodings_.push_back(GetEncodingById(ENCODING_ISO_8859_1));
    file_encodings_.push_back(GetEncodingById(ENCODING_UTF8));
    file_encodings_.push_back(GetEncodingById(ENCODING_UTF8_BOM));
    file_encodings_.push_back(GetEncodingById(ENCODING_UTF16_BE));
    file_encodings_.push_back(GetEncodingById(ENCODING_UTF16_LE));
    file_encodings_.push_back(GetEncodingById(ENCODING_GB18030));
    file_encodings_.push_back(GetEncodingById(ENCODING_BIG5));
  }
   
  void InitFileEncodingComboBox(wxComboBox* combo_box) {
    for (editor::Encoding& encoding : file_encodings_) {
      combo_box->Append(GetEncodingDisplayName(encoding.id));
    }
  }

  int IndexFileEncoding(editor::Encoding& encoding) const {
    for (size_t i = 0; i < file_encodings_.size(); ++i) {
      if (file_encodings_[i] == encoding) {
        return static_cast<int>(i);
      }
    }
    return wxNOT_FOUND;
  }

private:
  Options* options_;

  std::vector<editor::Encoding> file_encodings_;

  wxCheckBox* c_check_box_;
  wxCheckBox* j_check_box_;
  wxCheckBox* k_check_box_;

  wxComboBox* fenc_combo_box_;

  wxCheckBox* switch_cwd_check_box_;
  wxCheckBox* restore_files_check_box_;
  wxCheckBox* show_path_check_box_;

  wxSpinCtrl* line_padding_spin_ctrl_;
};

////////////////////////////////////////////////////////////////////////////////

class Global_FontPage : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  Global_FontPage(Options* options)
      : options_(options) {
    for (int i = 0; i < FONT_COUNT; ++i) {
      fonts_[i] = options_->fonts[i];
    }
  }

  virtual ~Global_FontPage() {
  }

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY) {
    if (!wxPanel::Create(parent, id)) {
      return false;
    }

    CreateControls();

    return true;
  }

  virtual bool TransferDataToWindow() override {
    TransferFontToWindow();
    return true;
  }

  virtual bool TransferDataFromWindow() override {
    for (int i = 0; i < FONT_COUNT; ++i) {
      options_->fonts[i] = fonts_[i];
    }
    return true;
  }

protected:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    CreateTypeSection(top_vsizer);
    CreateFontSection(top_vsizer);

    wxButton* reset_button = new wxButton(this, ID_FONT_RESET_BUTTON, wxT("Reset"));
    top_vsizer->Add(reset_button, wxSizerFlags().Right().Border(wxRIGHT));

    SetSizerAndFit(top_vsizer);
  }

  void CreateTypeSection(wxSizer* top_vsizer) {
    wxString font_types[FONT_COUNT] = {
      _("Text Editor"),
      _("Line Number"),
      _("Tabs"),
      _("Status Bar"),
    };

    wxStaticText* type_label = new wxStaticText(this, wxID_ANY, wxT("Font type:"));

    type_combo_box_ = CreateReadonlyComboBox(this, ID_FONT_TYPE_COMBOBOX);
    type_combo_box_->Append(FONT_COUNT, font_types);
    type_combo_box_->Select(FONT_TEXT);

    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(type_label, wxSizerFlags().Center());
    hsizer->Add(type_combo_box_, wxSizerFlags(1).Center().Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  void CreateFontSection(wxSizer* top_vsizer) {
    bool fixed_width_only = false;

    wxStaticText* name_label = new wxStaticText(this, wxID_ANY, wxT("Name:"));
    name_combo_box_ = new wxComboBox(this, ID_FONT_NAME_COMBOBOX, wxEmptyString, wxDefaultPosition, kStrTextSize);
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

  void InitNameComboBox(wxComboBox* combo_box, bool fixed_width_only) {
    FontEnumerator fe;
    fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, fixed_width_only);
    std::set<wxString>::iterator it = fe.facenames.begin();
    for (; it != fe.facenames.end(); ++it) {
      combo_box->Append(*it);
    }
  }

  void InitSizeComboBox(wxComboBox* combo_box) {
    for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
      combo_box->Append(wxString::Format(wxT("%d"), i));
    }
  }

  void SetFontToWindow(const wxFont& font) {
    name_combo_box_->SetValue(font.GetFaceName());

    wxString size_str = wxString::Format(wxT("%d"), font.GetPointSize());
    size_combo_box_->SetValue(size_str);
  }

  void TransferFontToWindow() {
    SetFontToWindow(fonts_[type_combo_box_->GetSelection()]);
  }

  bool TransferFontFromWindow() {
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

  void OnTypeComboBox(wxCommandEvent& evt) {
    TransferFontToWindow();
  }

  void OnNameComboBox(wxCommandEvent& evt) {
    TransferFontFromWindow();
  }

  void OnSizeComboBox(wxCommandEvent& evt) {
    TransferFontFromWindow();
  }

  void OnFixedWidthOnlyCheckBox(wxCommandEvent& evt) {
    wxString value = name_combo_box_->GetValue();

    name_combo_box_->Clear();
    InitNameComboBox(name_combo_box_, evt.IsChecked());

    name_combo_box_->SetValue(value);
  }

  void OnResetButton(wxCommandEvent& evt) {
    int type = type_combo_box_->GetSelection();
    fonts_[type] = options_->fonts[type];
    SetFontToWindow(fonts_[type]);
  }

private:
  Options* options_;

  wxFont fonts_[FONT_COUNT];

  wxComboBox* type_combo_box_;

  wxComboBox* name_combo_box_;
  wxComboBox* size_combo_box_;

  wxCheckBox* fixed_width_check_box_;
};

BEGIN_EVENT_TABLE(Global_FontPage, wxPanel)
EVT_COMBOBOX(ID_FONT_TYPE_COMBOBOX, Global_FontPage::OnTypeComboBox)
EVT_COMBOBOX(ID_FONT_NAME_COMBOBOX, Global_FontPage::OnNameComboBox)
EVT_COMBOBOX(ID_FONT_SIZE_COMBOBOX, Global_FontPage::OnSizeComboBox)
EVT_CHECKBOX(ID_FONT_FIXED_WIDTH_ONLY_CHECKBOX, Global_FontPage::OnFixedWidthOnlyCheckBox)
EVT_BUTTON(ID_FONT_RESET_BUTTON, Global_FontPage::OnResetButton)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

class Editor_GeneralPage : public wxPanel {
public:
  explicit Editor_GeneralPage(editor::Options* options)
      : options_(options) {
  }

  virtual ~Editor_GeneralPage() {
  }

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY) {
    if (!wxPanel::Create(parent, id)) {
      return false;
    }

    CreateControls();

    return true;
  }

  wxString JoinRulers(const std::vector<int>& rulers) {
    wxString rulers_str;
    for (int ruler : rulers) {
      if (!rulers_str.IsEmpty()) {
        rulers_str += wxT(",");
      }
      rulers_str += wxString::Format(wxT("%d"), ruler);
    }
    return rulers_str;
  }

  void SplitRulers(const wxString& rulers_str, std::vector<int>& rulers) {
    wxArrayString ruler_strs = wxSplit(rulers_str, wxT(','));
    long ruler = 0;
    for (size_t i = 0; i < ruler_strs.size(); ++i) {
      if (ruler_strs[i].ToLong(&ruler)) {
        if (ruler > 0) {
          rulers.push_back(static_cast<int>(ruler));
        }
      }
    }
  }

  virtual bool TransferDataToWindow() override {
    show_hscrollbar_check_box_->SetValue(options_->view.show_hscrollbar);
    show_number_check_box_->SetValue(options_->view.show_number);
    show_space_check_box_->SetValue(options_->view.show_space);
    wrap_check_box_->SetValue(options_->view.wrap);

    rulers_text_ctrl_->SetValue(JoinRulers(options_->view.rulers));

    wxString delimiters(options_->text.delimiters);
    delimiters_text_ctrl_->SetValue(delimiters);

    return true;
  }

  virtual bool TransferDataFromWindow() override {
    options_->view.show_hscrollbar = show_hscrollbar_check_box_->GetValue();
    options_->view.show_number = show_number_check_box_->GetValue();
    options_->view.show_space = show_space_check_box_->GetValue();
    options_->view.wrap = wrap_check_box_->GetValue();

    options_->view.rulers.clear();
    SplitRulers(rulers_text_ctrl_->GetValue(), options_->view.rulers);

    std::wstring delimiters = delimiters_text_ctrl_->GetValue().ToStdWstring();

    // Remove ' ' and '\t' and duplicate elements.
    std::sort(delimiters.begin(), delimiters.end());  // std::unique needs this.
    std::wstring::iterator end = std::remove(delimiters.begin(), delimiters.end(), L' ');
    end = std::remove(delimiters.begin(), end, L'\t');
    end = std::unique(delimiters.begin(), end);
    if (end != delimiters.end()) {
      delimiters.erase(end, delimiters.end());
    }

    options_->text.delimiters = delimiters;

    return true;
  }

protected:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    CreateDisplaySection(top_vsizer);
    CreateRulersSection(top_vsizer);
    CreateDelimitersSection(top_vsizer);

    SetSizerAndFit(top_vsizer);
  }

  void CreateDisplaySection(wxSizer* top_vsizer) {
    ui::StaticBox* box = new ui::StaticBox(this, _("Display"));
    wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

    show_hscrollbar_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show horizontal scrollbar"));
    box_vsizer->Add(show_hscrollbar_check_box_);

    show_number_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show line numbers"));
    box_vsizer->Add(show_number_check_box_, wxSizerFlags().Border(wxTOP));

    show_space_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show white spaces"));
    box_vsizer->Add(show_space_check_box_, wxSizerFlags().Border(wxTOP));

    wrap_check_box_ = new wxCheckBox(box, wxID_ANY, _("Wrap line"));
    box_vsizer->Add(wrap_check_box_, wxSizerFlags().Border(wxTOP));

    box->SetBodySizer(box_vsizer);
    top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));
  }

  void CreateRulersSection(wxSizer* top_vsizer) {
    wxStaticText* rulers_label = new wxStaticText(this, wxID_ANY, _("Rulers:"));
    rulers_text_ctrl_ = CreateTextCtrl(this, wxID_ANY);

    wxSizer* rulers_hsizer = new wxBoxSizer(wxHORIZONTAL);
    rulers_hsizer->Add(rulers_label, wxSizerFlags().Center());
    rulers_hsizer->AddStretchSpacer(1);
    rulers_hsizer->Add(rulers_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

    top_vsizer->Add(rulers_hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  void CreateDelimitersSection(wxSizer* top_vsizer) {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Delimiters:"));
    delimiters_text_ctrl_ = CreateTextCtrl(this, wxID_ANY, kStrTextSize);
    delimiters_text_ctrl_->SetToolTip(_("Word delimiters (exluding ' ' and '\t')."));

    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->Add(delimiters_text_ctrl_, wxSizerFlags(1).Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

private:
  editor::Options* options_;

  wxCheckBox* show_hscrollbar_check_box_;
  wxCheckBox* show_number_check_box_;
  wxCheckBox* show_space_check_box_;
  wxCheckBox* wrap_check_box_;
  wxTextCtrl* rulers_text_ctrl_;
  wxTextCtrl* delimiters_text_ctrl_;
};

////////////////////////////////////////////////////////////////////////////////

class Editor_IndentPage : public wxPanel {
public:
  explicit Editor_IndentPage(editor::Options* options)
      : options_(options) {
  }

  virtual ~Editor_IndentPage() {
  }

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY) {
    if (!wxPanel::Create(parent, id)) {
      return false;
    }

    CreateControls();

    return true;
  }

  virtual bool TransferDataToWindow() override {
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

  virtual bool TransferDataFromWindow() override {
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

protected:
  void CreateControls() {
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

  void CreateTabsSection(wxSizer* top_vsizer) {
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

  void CreateDynamicSection(wxSizer* top_vsizer) {
    top_vsizer->Add(CreateSeparator(this, _("Dynamic options"), true), wxSizerFlags().Expand().Border(wxALL));

    option_list_ctrl_ = new OptionListCtrl();
    option_list_ctrl_->Create(this, wxID_ANY, wxSize(-1, 120));

    top_vsizer->Add(option_list_ctrl_, wxSizerFlags().Expand().Border(wxALL));
  }

private:
  editor::Options* options_;

  wxTextCtrl* tab_stop_text_ctrl_;
  wxCheckBox* expand_tab_check_box_;
  wxCheckBox* guess_check_box_;
  wxTextCtrl* indent_keys_text_ctrl_;

  OptionListCtrl* option_list_ctrl_;
};


////////////////////////////////////////////////////////////////////////////////

class Editor_CommentPage : public wxPanel {
public:
  explicit Editor_CommentPage(editor::Options* options)
      : options_(options) {
  }

  virtual ~Editor_CommentPage() {
  }

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY) {
    if (!wxPanel::Create(parent, id)) {
      return false;
    }

    CreateControls();

    return true;
  }

  virtual bool TransferDataToWindow() override {
    add_space_check_box_->SetValue(options_->text.comment_add_space);
    respect_indent_check_box_->SetValue(options_->text.comment_respect_indent);

    return true;
  }

  virtual bool TransferDataFromWindow() override {
    options_->text.comment_add_space = add_space_check_box_->GetValue();
    options_->text.comment_respect_indent = respect_indent_check_box_->GetValue();

    return true;
  }

protected:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);
    
    ui::StaticBox* box = new ui::StaticBox(this, wxEmptyString);
    wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

    add_space_check_box_ = new wxCheckBox(box, wxID_ANY, _("Add space"));
    respect_indent_check_box_ = new wxCheckBox(box, wxID_ANY, _("Respect line indent"));
    
    box_vsizer->Add(add_space_check_box_, wxSizerFlags().Border(wxTOP));
    box_vsizer->Add(respect_indent_check_box_, wxSizerFlags().Expand().Border(wxTOP));

    box->SetBodySizer(box_vsizer);
    top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));

    SetSizerAndFit(top_vsizer);
  }

private:
  editor::Options* options_;

  wxCheckBox* add_space_check_box_;
  wxCheckBox* respect_indent_check_box_;
};

}  // namespace pref

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(PrefDialogBase, wxDialog)
EVT_BUTTON(wxID_OK,       PrefDialogBase::OnButtonOK)
EVT_BUTTON(wxID_CANCEL,   PrefDialogBase::OnButtonCancel)
END_EVENT_TABLE()

PrefDialogBase::PrefDialogBase() : notebook_(NULL) {
}

PrefDialogBase::~PrefDialogBase() {
}

bool PrefDialogBase::Create(wxWindow* parent, wxWindowID id, const wxString& title) {
  // Recursively call TransferDataFromWindow() and TransferDataToWindow().
  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);

  if (!wxDialog::Create(parent, id, title)) {
    return false;
  }

  notebook_ = new wxNotebook(this, wxID_ANY);

  AddPages();

  // Layout
  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(notebook_, wxSizerFlags().Expand().Border(wxALL));

  wxSizer* button_sizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
  // NOTE: No left/right border, the button sizer has considered it.
  vsizer->Add(button_sizer, wxSizerFlags().Expand().Border(wxTOP | wxBOTTOM));

  SetSizer(vsizer);

  Fit();

  return true;
}

void PrefDialogBase::OnButtonOK(wxCommandEvent& evt) {
  TransferDataFromWindow();
  EndModal(wxID_OK);
}

void PrefDialogBase::OnButtonCancel(wxCommandEvent& evt) {
  EndModal(wxID_CANCEL);
}

////////////////////////////////////////////////////////////////////////////////

PrefGlobalDialog::PrefGlobalDialog(Options* options)
    : options_(options) {
}

PrefGlobalDialog::~PrefGlobalDialog() {
}

void PrefGlobalDialog::AddPages() {
  notebook_->AddPage(CreateGeneralPage(), _("General"), true);
  notebook_->AddPage(CreateFontPage(), _("Font"), false);
}

wxWindow* PrefGlobalDialog::CreateGeneralPage() {
  pref::Global_GeneralPage* page = new pref::Global_GeneralPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

wxWindow* PrefGlobalDialog::CreateFontPage() {
  pref::Global_FontPage* page = new pref::Global_FontPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

////////////////////////////////////////////////////////////////////////////////

PrefEditorDialog::PrefEditorDialog(editor::Options* options)
    : options_(options) {
}

PrefEditorDialog::~PrefEditorDialog() {
}

void PrefEditorDialog::AddPages() {
  notebook_->AddPage(CreateGeneralPage(), _("General"), true);
  notebook_->AddPage(CreateIndentPage(), _("Indent"), false);
  notebook_->AddPage(CreateCommentPage(), _("Comment"), false);
}

wxWindow* PrefEditorDialog::CreateGeneralPage() {
  pref::Editor_GeneralPage* page = new pref::Editor_GeneralPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

wxWindow* PrefEditorDialog::CreateIndentPage() {
  pref::Editor_IndentPage* page = new pref::Editor_IndentPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

wxWindow* PrefEditorDialog::CreateCommentPage() {
  pref::Editor_CommentPage* page = new pref::Editor_CommentPage(options_);

  wxColour theme_bg_colour = notebook_->GetThemeBackgroundColour();
  if (theme_bg_colour.IsOk()) {
    page->SetBackgroundColour(theme_bg_colour);
  }

  page->Create(notebook_);
  return page;
}

}  // namespace jil
