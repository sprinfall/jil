#include "app/preference_pages.h"

#include "wx/button.h"
#include "wx/bookctrl.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/msgdlg.h"  // TEST
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/statline.h"
#include "wx/stattext.h"

#include "editor/defs.h"

#include "app/app.h"
#include "app/font_util.h"
#include "app/option.h"

namespace jil {

static const wxSize kMinComboBoxSize(120, -1);
static const wxSize kNumTextSize(60, -1);
static const wxSize kStrTextSize(180, -1);

////////////////////////////////////////////////////////////////////////////////

class GeneralPagePanel : public wxPanel {
public:
  GeneralPagePanel(Options* options)
      : options_(options) {
  }

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY) {
    if (!wxPanel::Create(parent, id)) {
      return false;
    }

    CreateControls();
    return true;
  }

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

private:
  void CreateControls();

  void InitFileEncodingComboBox(wxComboBox* combo_box);

private:
  Options* options_;

  // CJK
  wxCheckBox* c_check_box_;
  wxCheckBox* j_check_box_;
  wxCheckBox* k_check_box_;

  wxComboBox* fenc_combo_box_;

  wxCheckBox* switch_cwd_check_box_;
  wxCheckBox* restore_files_check_box_;
  wxCheckBox* show_path_check_box_;
};

void GeneralPagePanel::CreateControls() {
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

      c_check_box_ = new wxCheckBox(box, wxID_ANY, _("Chinese"));
      j_check_box_ = new wxCheckBox(box, wxID_ANY, _("Japanese"));
      k_check_box_ = new wxCheckBox(box, wxID_ANY, _("Korean"));

      wxBoxSizer* cjk_check_hsizer = new wxBoxSizer(wxHORIZONTAL);
      cjk_check_hsizer->Add(c_check_box_);
      cjk_check_hsizer->Add(j_check_box_, wxSizerFlags().Border(wxLEFT));
      cjk_check_hsizer->Add(k_check_box_, wxSizerFlags().Border(wxLEFT));

      wxBoxSizer* cjk_vsizer = new wxBoxSizer(wxVERTICAL);
      cjk_vsizer->Add(cjk_label);
      cjk_vsizer->Add(cjk_check_hsizer, wxSizerFlags().Expand().Border(wxTOP));

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

  SetSizerAndFit(top_vsizer);
}

bool GeneralPagePanel::TransferDataToWindow() {
  switch_cwd_check_box_->SetValue(options_->switch_cwd);
  restore_files_check_box_->SetValue(options_->restore_files);
  show_path_check_box_->SetValue(options_->show_path);

  return true;
}

bool GeneralPagePanel::TransferDataFromWindow() {
  //wxCommandEvent dummy;
  //ChangedUseMarkdown(dummy);
  //ChangedSpellcheck(dummy);
  return true;
}

void GeneralPagePanel::InitFileEncodingComboBox(wxComboBox* combo_box) {
  combo_box->Append(ENCODING_DISPLAY_NAME_ISO_8859_1);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF8);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF8_BOM);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF16_BE);
  combo_box->Append(ENCODING_DISPLAY_NAME_UTF16_LE);
  combo_box->Append(ENCODING_DISPLAY_NAME_GB18030);
  combo_box->Append(ENCODING_DISPLAY_NAME_BIG5);
}

////////////////////////////////////////////////////////////////////////////////

class ThemePagePanel : public wxPanel {
public:
  ThemePagePanel(wxWindow* parent, wxWindowID id = wxID_ANY)
      : wxPanel(parent, id) {
    CreateControls();
  }

private:
  void CreateControls();

  void InitThemeComboBox(wxComboBox* combo_box);

  void InitFontNameComboBox(wxComboBox* combo_box, bool fixed_width_only);
  void InitFontSizeComboBox(wxComboBox* combo_box);
};

void ThemePagePanel::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  //----------------------------------------------------------------------------
  // Theme

  {
    wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Theme"));
    wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

    {
      wxComboBox* combo_box = new wxComboBox(box, wxID_ANY, wxEmptyString);
      InitThemeComboBox(combo_box);
      combo_box->SetMinSize(kMinComboBoxSize);

      box_vsizer->Add(combo_box, wxSizerFlags().Border(wxALL));
    }

    top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //----------------------------------------------------------------------------
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

    //----------------------------------------------------------------------------

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

  //----------------------------------------------------------------------------

  SetSizerAndFit(top_vsizer);
}

void ThemePagePanel::InitThemeComboBox(wxComboBox* combo_box) {
  const std::list<wxString>& theme_names = wxGetApp().theme_names();
  std::list<wxString>::const_iterator it = theme_names.begin();
  for (; it != theme_names.end(); ++it) {
    combo_box->Append(*it);
  }
}

void ThemePagePanel::InitFontNameComboBox(wxComboBox* combo_box, bool fixed_width_only) {
  FontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, fixed_width_only);
  std::set<wxString>::iterator it = fe.facenames.begin();
  for (; it != fe.facenames.end(); ++it) {
    combo_box->Append(*it);
  }
}

void ThemePagePanel::InitFontSizeComboBox(wxComboBox* combo_box) {
  for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
    combo_box->Append(wxString::Format(wxT("%d"), i));
  }
}

////////////////////////////////////////////////////////////////////////////////

class AdvancedEditorOptionsDialog : public wxDialog {
public:
  AdvancedEditorOptionsDialog(wxWindow* parent)
      : wxDialog(parent, wxID_ANY, _("Editor Options")) {
    CreateControls();
  }

protected:
  void CreateControls() {
    wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

    //------------------------------------
    // Comment

    {
      wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Comment"));
      wxSizer* box_hsizer = new wxStaticBoxSizer(box, wxHORIZONTAL);

      add_space_check_box_ = new wxCheckBox(box, wxID_ANY, _("Add space"));
      respect_indent_check_box_ = new wxCheckBox(box, wxID_ANY, _("Respect indent"));

      box_hsizer->Add(add_space_check_box_, wxSizerFlags().Border(wxALL));
      box_hsizer->Add(respect_indent_check_box_, wxSizerFlags().Border(wxALL));

      top_vsizer->Add(box_hsizer, wxSizerFlags().Expand().Border(wxTB));
    }

    //------------------------------------
    // Indent

    {
      wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Indent"));
      wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

      wxStaticText* sw_label = new wxStaticText(box, wxID_ANY, _("Shift width:"));
      sw_text_ctrl_ = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kNumTextSize);

      {
        wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
        hsizer->Add(sw_label, wxSizerFlags().Center());
        hsizer->Add(sw_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));
        box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
      }

      wxStaticText* indent_keys_label = new wxStaticText(box, wxID_ANY, _("Indent keys:"));
      indent_keys_text_ctrl_ = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kStrTextSize);

      {
        wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
        hsizer->Add(indent_keys_label, wxSizerFlags().Center());
        hsizer->Add(indent_keys_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));
        box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
      }

      top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxTB));
    }

    SetSizerAndFit(top_vsizer);
  }

private:
  wxCheckBox* add_space_check_box_;
  wxCheckBox* respect_indent_check_box_;

  wxTextCtrl* sw_text_ctrl_;
  wxTextCtrl* indent_keys_text_ctrl_;
};

class EditorPagePanel : public wxPanel {
public:
  EditorPagePanel(wxWindow* parent, wxWindowID id = wxID_ANY)
      : wxPanel(parent, id) {
    CreateControls();
  }

protected:
  void CreateControls();

  void InitFtComboBox(wxComboBox* combo_box);

  void OnFtComboBox(wxCommandEvent& evt);

  void OnMoreButtonClick(wxCommandEvent& evt);

private:
  wxComboBox* ft_combo_box_;

  wxCheckBox* show_hscrollbar_check_box_;
  wxCheckBox* show_number_check_box_;
  wxCheckBox* show_space_check_box_;
  wxCheckBox* wrap_check_box_;

  wxTextCtrl* rulers_text_ctrl_;

  wxTextCtrl* delimiters_text_ctrl_;

  wxTextCtrl* ts_text_ctrl_;
  wxCheckBox* expand_tab_check_box_;
};

void EditorPagePanel::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  //------------------------------------
  // File type

  wxStaticText* ft_label = new wxStaticText(this, wxID_ANY, _("File type:"));

  ft_combo_box_ = new wxComboBox(this, wxID_ANY, wxEmptyString);
  InitFtComboBox(ft_combo_box_);

  wxBoxSizer* ft_hsizer = new wxBoxSizer(wxHORIZONTAL);
  ft_hsizer->AddStretchSpacer();
  ft_hsizer->Add(ft_label, wxSizerFlags().Center());
  ft_hsizer->Add(ft_combo_box_, wxSizerFlags().Center().Border(wxLEFT));
  top_vsizer->Add(ft_hsizer, wxSizerFlags().Expand().Border(wxALL));

  //------------------------------------
  // Display

  {
    wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Display"));
    wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

    show_hscrollbar_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show horizontal scrollbar"));
    box_vsizer->Add(show_hscrollbar_check_box_, wxSizerFlags().Border(wxLTR));

    show_number_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show line numbers"));
    box_vsizer->Add(show_number_check_box_, wxSizerFlags().Border(wxLTR));

    show_space_check_box_ = new wxCheckBox(box, wxID_ANY, _("Show white spaces"));
    box_vsizer->Add(show_space_check_box_, wxSizerFlags().Border(wxLTR));

    wrap_check_box_ = new wxCheckBox(box, wxID_ANY, _("Wrap line"));
    box_vsizer->Add(wrap_check_box_, wxSizerFlags().Border(wxALL));

    top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxLR));
  }

  //------------------------------------
  // Rulers

  wxStaticText* rulers_label = new wxStaticText(this, wxID_ANY, _("Rulers:"));
  rulers_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

  wxSizer* rulers_hsizer = new wxBoxSizer(wxHORIZONTAL);
  rulers_hsizer->Add(rulers_label, wxSizerFlags().Center());
  rulers_hsizer->AddStretchSpacer(1);
  rulers_hsizer->Add(rulers_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));
  top_vsizer->Add(rulers_hsizer, wxSizerFlags().Expand().Border(wxALL));

  //------------------------------------
  // Delimiters

  wxStaticText* delimiters_label = new wxStaticText(this, wxID_ANY, _("Delimiters:"));
  wxTextCtrl* delimiters_text_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
  {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(delimiters_label, wxSizerFlags().Center());
    hsizer->Add(delimiters_text_ctrl_, wxSizerFlags(1).Center().Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //------------------------------------
  // Tab

  {
    wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Tab"));
    wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

    {
      wxStaticText* ts_label = new wxStaticText(box, wxID_ANY, _("Tab stop:"));
      ts_text_ctrl_ = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kNumTextSize);

      expand_tab_check_box_ = new wxCheckBox(box, wxID_ANY, _("Expand tabs"));

      wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(ts_label, wxSizerFlags().Center());
      hsizer->Add(ts_text_ctrl_, wxSizerFlags().Center().Border(wxLEFT));
      hsizer->AddStretchSpacer(1);
      hsizer->Add(expand_tab_check_box_, wxSizerFlags().Center().Border(wxLEFT));
      box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  wxButton* more_button = new wxButton(this, wxID_ANY, _("More..."));
  top_vsizer->Add(more_button, wxSizerFlags().Right().Border(wxALL));
  Connect(more_button->GetId(), wxEVT_BUTTON, wxCommandEventHandler(EditorPagePanel::OnMoreButtonClick));

  SetSizerAndFit(top_vsizer);
}

// TODO
void EditorPagePanel::InitFtComboBox(wxComboBox* combo_box) {
  combo_box->Append(_T("C++"));
  combo_box->Append(_T("Java"));
  combo_box->Append(_T("Python"));
  combo_box->Select(0);
}

void EditorPagePanel::OnFtComboBox(wxCommandEvent& evt) {
  wxMessageBox(wxString::Format(wxT("selection: %d"), evt.GetSelection()));
}

void EditorPagePanel::OnMoreButtonClick(wxCommandEvent& evt) {
  AdvancedEditorOptionsDialog* more_dialog = new AdvancedEditorOptionsDialog(this);
  more_dialog->ShowModal();
}

////////////////////////////////////////////////////////////////////////////////

namespace pref {

wxWindow* GeneralPage::CreateWindow(wxWindow *parent) {
  GeneralPagePanel* panel = new GeneralPagePanel(options_);
  panel->Create(parent);
  return panel;
}

wxWindow* ThemePage::CreateWindow(wxWindow *parent) {
  return new ThemePagePanel(parent);
}

wxWindow* EditorPage::CreateWindow(wxWindow *parent) {
  return new EditorPagePanel(parent);
}

}  // namespace pref

}  // namespace jil