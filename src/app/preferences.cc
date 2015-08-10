#include "app/preferences.h"

#include <vector>

#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"
#include "wx/stattext.h"

#include "ui/label_ctrl.h"
#include "ui/static_box.h"

#include "editor/defs.h"

#include "app/defs.h"
#include "app/font_util.h"
#include "app/option.h"

namespace jil {

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

static wxComboBox* CreateReadonlyComboBox(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString) {
  return new wxComboBox(parent, id, value, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
}

namespace pref {

////////////////////////////////////////////////////////////////////////////////

enum Id {
  ID_PREF_BEGIN = wxID_HIGHEST + 100,

  ID_FONT_LABEL_CTRL,
  ID_GUI_FONT_LABEL_CTRL,
  ID_FIXED_WIDTH_ONLY_CHECKBOX,
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

    // NOTE:
    // Don't separate Chinese to Simplified and Traditional. Keep it simple.
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
    editor::Encoding encoding;

    encoding.value = wxFONTENCODING_ISO8859_1;
    encoding.name = editor::ENCODING_NAME_ISO_8859_1;
    encoding.display_name = ENCODING_DISPLAY_NAME_ISO_8859_1;
    file_encodings_.push_back(encoding);

    encoding.value = wxFONTENCODING_UTF8;
    encoding.name = editor::ENCODING_NAME_UTF8;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF8;
    file_encodings_.push_back(encoding);

    encoding.value = wxFONTENCODING_UTF8;
    encoding.name = editor::ENCODING_NAME_UTF8_BOM;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF8_BOM;
    file_encodings_.push_back(encoding);

    encoding.value = wxFONTENCODING_UTF16BE;
    encoding.name = editor::ENCODING_NAME_UTF16_BE;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF16_BE;
    file_encodings_.push_back(encoding);

    encoding.value = wxFONTENCODING_UTF16LE;
    encoding.name = editor::ENCODING_NAME_UTF16_LE;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF16_LE;
    file_encodings_.push_back(encoding);

    encoding.value = wxFONTENCODING_GB2312;
    encoding.name = editor::ENCODING_NAME_GB18030;
    encoding.display_name = ENCODING_DISPLAY_NAME_GB18030;
    file_encodings_.push_back(encoding);

    encoding.value = wxFONTENCODING_BIG5;
    encoding.name = editor::ENCODING_NAME_BIG5;
    encoding.display_name = ENCODING_DISPLAY_NAME_BIG5;
    file_encodings_.push_back(encoding);
  }

  void InitFileEncodingComboBox(wxComboBox* combo_box) {
    for (editor::Encoding& encoding : file_encodings_) {
      combo_box->Append(encoding.display_name);
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

  wxButton* fonts_button_;
};

////////////////////////////////////////////////////////////////////////////////

// Dialog for selecting font name and size.
class SelectFontDialog : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  SelectFontDialog(const wxFont& font, bool fixed_width_only)
      : font_(font), fixed_width_only_(fixed_width_only) {
  }

  virtual ~SelectFontDialog() {
  }

  const wxFont& font() const {
    return font_;
  }

  bool Create(wxWindow* parent, wxWindowID id, const wxString& title) {
    if (!wxDialog::Create(parent, id, title)) {
      return false;
    }

    wxBusyCursor wait;

    CreateControls();

    return true;
  }

protected:
  virtual bool TransferDataToWindow() override {
    name_combo_box_->SetValue(font_.GetFaceName());

    wxString size_str = wxString::Format(wxT("%d"), font_.GetPointSize());
    size_combo_box_->SetValue(size_str);

    fixed_width_check_box_->SetValue(fixed_width_only_);

    return true;
  }

  virtual bool TransferDataFromWindow() override {
    wxString name = name_combo_box_->GetValue();
    if (!font_.SetFaceName(name)) {
      return false;
    }

    long size = kDefaultFontSize;
    if (!size_combo_box_->GetValue().ToLong(&size)) {
      return false;
    }
    if (size < kMinFontSize || size > kMaxFontSize) {
      size = kDefaultFontSize;
    }
    font_.SetPointSize(static_cast<int>(size));

    return true;
  }

  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    name_combo_box_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, kStrTextSize);
    InitNameComboBox(name_combo_box_, fixed_width_only_);

    size_combo_box_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, kNumTextSize);
    InitSizeComboBox(size_combo_box_);

    fixed_width_check_box_ = new wxCheckBox(this, ID_FIXED_WIDTH_ONLY_CHECKBOX, wxT("Fixed width only"));

    // Layout

    top_vsizer->Add(fixed_width_check_box_, wxSizerFlags().Expand().Border(wxALL));

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(name_combo_box_, wxSizerFlags(3));
    hsizer->Add(size_combo_box_, wxSizerFlags(1).Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));

    top_vsizer->AddSpacer(10);

    // OK & Cancel buttons

    wxSizer* button_sizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    // NOTE: No left/right border, the button sizer has considered it.
    top_vsizer->Add(button_sizer, wxSizerFlags().Expand().Border(wxTOP | wxBOTTOM));

    SetSizerAndFit(top_vsizer);
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

  void OnFixedWidthOnlyCheckBox(wxCommandEvent& evt) {
    name_combo_box_->Clear();
    InitNameComboBox(name_combo_box_, evt.IsChecked());
    name_combo_box_->SetValue(font_.GetFaceName());
  }

  void OnButtonOK(wxCommandEvent& evt) {
    TransferDataFromWindow();
    EndModal(wxID_OK);
  }

  void OnButtonCancel(wxCommandEvent& evt) {
    EndModal(wxID_CANCEL);
  }

private:
  wxFont font_;
  bool fixed_width_only_;

  wxComboBox* name_combo_box_;
  wxComboBox* size_combo_box_;

  wxCheckBox* fixed_width_check_box_;
};

BEGIN_EVENT_TABLE(SelectFontDialog, wxDialog)
EVT_CHECKBOX(ID_FIXED_WIDTH_ONLY_CHECKBOX, SelectFontDialog::OnFixedWidthOnlyCheckBox)
EVT_BUTTON(wxID_OK, SelectFontDialog::OnButtonOK)
EVT_BUTTON(wxID_CANCEL, SelectFontDialog::OnButtonCancel)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

class Global_FontPage : public wxPanel {
public:
  Global_FontPage(Options* options) : options_(options) {
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

protected:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    // Create controls

    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Font"));
    font_label_ctrl_ = new ui::LabelCtrl(this, ID_FONT_LABEL_CTRL, GetFontLabel(options_->font));

    wxStaticText* gui_label = new wxStaticText(this, wxID_ANY, _("GUI font"));
    gui_font_label_ctrl_ = new ui::LabelCtrl(this, ID_GUI_FONT_LABEL_CTRL, GetFontLabel(options_->gui_font));

    // Layout

    top_vsizer->Add(LayoutFont(label, font_label_ctrl_), wxSizerFlags().Expand().Border(wxLTR));
    top_vsizer->Add(LayoutFont(gui_label, gui_font_label_ctrl_), wxSizerFlags().Expand().Border(wxLTR));

    // Connect event handlers

    Connect(ID_FONT_LABEL_CTRL,
            wxEVT_BUTTON,
            wxCommandEventHandler(Global_FontPage::OnFontLabelClick));

    Connect(ID_GUI_FONT_LABEL_CTRL,
            wxEVT_BUTTON,
            wxCommandEventHandler(Global_FontPage::OnGuiFontLabelClick));

    SetSizerAndFit(top_vsizer);
  }

  wxSizer* LayoutFont(wxStaticText* text, ui::LabelCtrl* label_ctrl) {
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(text, wxSizerFlags().Center());
    hsizer->AddStretchSpacer();
    hsizer->Add(label_ctrl, wxSizerFlags().Center().Border(wxLEFT));
    return hsizer;
  }

  wxString GetFontLabel(const wxFont& font) const {
    if (!font.IsOk()) {
      return wxT("N/A");
    }
    return font.GetFaceName() + wxString::Format(wxT(", %d"), font.GetPointSize());
  }

  void OnFontLabelClick(wxCommandEvent& evt) {
    if (ShowFontDialog(options_->font, true)) {
      font_label_ctrl_->SetLabel(GetFontLabel(options_->font));
      GetSizer()->Layout();
      font_label_ctrl_->Refresh();
    }
  }

  void OnGuiFontLabelClick(wxCommandEvent& evt) {
    if (ShowFontDialog(options_->gui_font, false)) {
      gui_font_label_ctrl_->SetLabel(GetFontLabel(options_->gui_font));
      GetSizer()->Layout();
      gui_font_label_ctrl_->Refresh();
    }
  }

  // Return true if the font is changed.
  bool ShowFontDialog(wxFont& font, bool fixed_width_only) {
    SelectFontDialog dialog(font, fixed_width_only);
    dialog.Create(this, wxID_ANY, _("Font"));

    if (dialog.ShowModal() != wxID_OK) {
      return false;
    }

    if (font == dialog.font()) {
      return false;
    }

    font = dialog.font();
    return true;
  }

private:
  Options* options_;

  ui::LabelCtrl* font_label_ctrl_;
  ui::LabelCtrl* gui_font_label_ctrl_;
};

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
    rulers_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

    wxSizer* rulers_hsizer = new wxBoxSizer(wxHORIZONTAL);
    rulers_hsizer->Add(rulers_label, wxSizerFlags().Center());
    rulers_hsizer->AddStretchSpacer(1);
    rulers_hsizer->Add(rulers_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

    top_vsizer->Add(rulers_hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  void CreateDelimitersSection(wxSizer* top_vsizer) {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Delimiters:"));
    delimiters_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, kStrTextSize);

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

protected:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    CreateTabsSection(top_vsizer);

    wxStaticText* indent_keys_label = new wxStaticText(this, wxID_ANY, _("Indent keys:"));
    indent_keys_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, kStrTextSize);
    {
      wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(indent_keys_label, wxSizerFlags().Center());
      hsizer->Add(indent_keys_text_ctrl_, wxSizerFlags(1).Center().Border(wxLEFT));

      top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    // More indent options.
    indent_list_ctrl_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
    indent_list_ctrl_->AppendColumn(_("Key"));
    indent_list_ctrl_->AppendColumn(_("Value"));
    top_vsizer->Add(indent_list_ctrl_, wxSizerFlags().Expand().Border(wxALL));

    SetSizerAndFit(top_vsizer);
  }

  void CreateTabsSection(wxSizer* top_vsizer) {
    ui::StaticBox* box = new ui::StaticBox(this, _("Tabs"));
    wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* tab_stop_label = new wxStaticText(box, wxID_ANY, _("Tab stop:"));
    tab_stop_text_ctrl_ = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kNumTextSize);

    wxStaticText* shift_width_label = new wxStaticText(box, wxID_ANY, _("Shift width:"));
    shift_width_text_ctrl_ = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kNumTextSize);

    expand_tab_check_box_ = new wxCheckBox(box, wxID_ANY, _("Expand tabs"));

    {
      wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(tab_stop_label, wxSizerFlags().Center());
      hsizer->Add(tab_stop_text_ctrl_, wxSizerFlags().Center().Border(wxLR));
      hsizer->AddStretchSpacer(1);
      hsizer->Add(shift_width_label, wxSizerFlags().Center().Border(wxLEFT));
      hsizer->Add(shift_width_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

      box_vsizer->Add(hsizer, wxSizerFlags().Expand());
    }

    box_vsizer->Add(expand_tab_check_box_, wxSizerFlags().Border(wxTOP));

    box->SetBodySizer(box_vsizer);
    top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));
  }

private:
  editor::Options* options_;

  wxTextCtrl* tab_stop_text_ctrl_;
  wxTextCtrl* shift_width_text_ctrl_;
  wxCheckBox* expand_tab_check_box_;

  wxTextCtrl* indent_keys_text_ctrl_;
  wxListCtrl* indent_list_ctrl_;
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

}  // namespace jil
