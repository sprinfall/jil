#include "app/preferences.h"

#include <vector>

#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"

#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/spinctrl.h"
#include "wx/statbox.h"
#include "wx/stattext.h"

#include "ui/static_box.h"

#include "editor/defs.h"

#include "app/defs.h"
#include "app/font_util.h"
#include "app/option.h"

namespace jil {
namespace pref {

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

wxComboBox* CreateReadonlyComboBox(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString) {
  return new wxComboBox(parent, id, value, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
}

////////////////////////////////////////////////////////////////////////////////

class Global_GeneralPanel : public wxPanel {
public:
  Global_GeneralPanel(Options* options) : options_(options) {
    InitFileEncodingTable();
  }

  virtual ~Global_GeneralPanel() {
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

    //--------------------------------------------------------------------------
    // Encoding

    {
      wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Encoding"));
      wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

      //------------------------------------------------------------------------
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

      //------------------------------------------------------------------------
      // File Encoding

      {
        wxStaticText* label = new wxStaticText(box, wxID_ANY, _("Default file encoding:"));

        fenc_combo_box_ = CreateReadonlyComboBox(box, wxID_ANY);
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

    //--------------------------------------------------------------------------

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

    //--------------------------------------------------------------------------
    // Line padding

    {
      wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Line padding:"));
      line_padding_spin_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
      line_padding_spin_ctrl_->SetRange(kMinLinePadding, kMaxLinePadding);

      wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(label, wxSizerFlags().Center());
      hsizer->AddStretchSpacer(1);
      hsizer->Add(line_padding_spin_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

      top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    //--------------------------------------------------------------------------

    SetSizerAndFit(top_vsizer);
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

wxWindow* Global_GeneralPage::CreateWindow(wxWindow *parent) {
  Global_GeneralPanel* panel = new Global_GeneralPanel(options_);
  panel->Create(parent);
  return panel;
}

////////////////////////////////////////////////////////////////////////////////

class Global_FontPanel : public wxPanel {
public:
  Global_FontPanel(Options* options) : options_(options) {
  }

  virtual ~Global_FontPanel() {
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

    //--------------------------------------------------------------------------
    // Font

    {
      wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Font"));
      wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

      // Font Area
      {
        wxStaticText* label = new wxStaticText(box, wxID_ANY, _("Area:"));

        wxComboBox* combo_box = new wxComboBox(box, wxID_ANY, wxEmptyString);
        combo_box->SetMinSize(kMinComboBoxSize);
        combo_box->Append(_("Text Editor"));
        combo_box->Append(_("Tab"));
        combo_box->Append(_("Status Bar"));
        combo_box->Select(0);

        wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
        hsizer->AddStretchSpacer();
        hsizer->Add(label, wxSizerFlags().Center());
        hsizer->Add(combo_box, wxSizerFlags().Center().Border(wxLEFT));

        box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
      }

      //--------------------------------------------------------------------------

      // Font Name & Size
      {
        wxStaticText* name_label = new wxStaticText(box, wxID_ANY, _("Font:"));
        wxComboBox* name_combo_box = new wxComboBox(box, wxID_ANY, wxEmptyString);
        InitFontNameComboBox(name_combo_box, false);

        wxStaticText* size_label = new wxStaticText(box, wxID_ANY, _("Size:"));
        wxComboBox* size_combo_box = new wxComboBox(box, wxID_ANY, wxEmptyString);
        InitFontSizeComboBox(size_combo_box);

        wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

        wxBoxSizer* name_vsizer = new wxBoxSizer(wxVERTICAL);
        name_vsizer->Add(name_label);
        name_vsizer->Add(name_combo_box, wxSizerFlags().Expand());

        wxBoxSizer* size_vsizer = new wxBoxSizer(wxVERTICAL);
        size_vsizer->Add(size_label);
        size_vsizer->Add(size_combo_box, wxSizerFlags().Expand());

        hsizer->Add(name_vsizer, wxSizerFlags(3));
        hsizer->Add(size_vsizer, wxSizerFlags(1).Border(wxLEFT));

        box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
      }

      top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    //--------------------------------------------------------------------------

    SetSizerAndFit(top_vsizer);
  }

  void InitFontNameComboBox(wxComboBox* combo_box, bool fixed_width_only) {
    FontEnumerator fe;
    fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, fixed_width_only);
    std::set<wxString>::iterator it = fe.facenames.begin();
    for (; it != fe.facenames.end(); ++it) {
      combo_box->Append(*it);
    }
  }

  void InitFontSizeComboBox(wxComboBox* combo_box) {
    for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
      combo_box->Append(wxString::Format(wxT("%d"), i));
    }
  }

private:
  Options* options_;
};

wxWindow* Global_FontPage::CreateWindow(wxWindow *parent) {
  Global_FontPanel* panel = new Global_FontPanel(options_);
  panel->Create(parent);
  return panel;
}

////////////////////////////////////////////////////////////////////////////////

class Editor_GeneralPanel : public wxPanel {
public:
  Editor_GeneralPanel(editor::Options* options) : options_(options) {
  }

  virtual ~Editor_GeneralPanel() {
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
    CreateTabSection(top_vsizer);

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
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Rulers:"));
    rulers_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->AddStretchSpacer(1);
    hsizer->Add(rulers_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  void CreateDelimitersSection(wxSizer* top_vsizer) {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Delimiters:"));
    delimiters_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, kStrTextSize);

    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->Add(delimiters_text_ctrl_, wxSizerFlags(1).Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  void CreateTabSection(wxSizer* top_vsizer) {
    ui::StaticBox* box = new ui::StaticBox(this, _("Tab"));
    wxSizer* box_vsizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* ts_label = new wxStaticText(box, wxID_ANY, _("Tab stop:"));
    ts_text_ctrl_ = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kNumTextSize);

    expand_tab_check_box_ = new wxCheckBox(box, wxID_ANY, _("Expand tabs"));

    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(ts_label, wxSizerFlags().Center());
    hsizer->Add(ts_text_ctrl_, wxSizerFlags().Center().Border(wxLR));
    hsizer->AddStretchSpacer(1);
    hsizer->Add(expand_tab_check_box_, wxSizerFlags().Center().Border(wxLEFT));

    box_vsizer->Add(hsizer, wxSizerFlags().Expand());

    box->SetBodySizer(box_vsizer);
    top_vsizer->Add(box, wxSizerFlags().Expand().Border(wxALL));
  }

private:
  editor::Options* options_;

  wxCheckBox* show_hscrollbar_check_box_;
  wxCheckBox* show_number_check_box_;
  wxCheckBox* show_space_check_box_;
  wxCheckBox* wrap_check_box_;

  wxTextCtrl* rulers_text_ctrl_;

  wxTextCtrl* delimiters_text_ctrl_;

  wxTextCtrl* ts_text_ctrl_;
  wxCheckBox* expand_tab_check_box_;
};

wxWindow* Editor_GeneralPage::CreateWindow(wxWindow *parent) {
  Editor_GeneralPanel* panel = new Editor_GeneralPanel(options_);
  panel->Create(parent);
  return panel;
}

}  // namespace pref
}  // namespace jil
