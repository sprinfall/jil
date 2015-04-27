#include "app/preference_pages.h"

#include "wx/button.h"
#include "wx/bookctrl.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
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

  void FillFileEncodingComboBox(wxComboBox* fenc_combo);

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
      FillFileEncodingComboBox(fenc_combo_box_);

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
    restore_files_check_box_ = new wxCheckBox(this, wxID_ANY, _("Remember last open files"));
    top_vsizer->Add(restore_files_check_box_, wxSizerFlags().Border(wxLTR));
  }
  
  {
    show_path_check_box_ = new wxCheckBox(this, wxID_ANY, _("Show file full path in the title bar"));
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

void GeneralPagePanel::FillFileEncodingComboBox(wxComboBox* combo_box) {
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

  void FillThemeComboBox(wxComboBox* combo_box);

  void FillFontNameComboBox(wxComboBox* combo_box, bool fixed_width_only);
  void FillFontSizeComboBox(wxComboBox* combo_box);
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
      FillThemeComboBox(combo_box);
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
      FillFontNameComboBox(name_combo_box, false);

      wxStaticText* size_label = new wxStaticText(box, wxID_ANY, _("Size:"));
      wxComboBox* size_combo_box = new wxComboBox(box, wxID_ANY, wxEmptyString);
      FillFontSizeComboBox(size_combo_box);

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

void ThemePagePanel::FillThemeComboBox(wxComboBox* theme_combo) {
  const std::list<wxString>& theme_names = wxGetApp().theme_names();
  std::list<wxString>::const_iterator it = theme_names.begin();
  for (; it != theme_names.end(); ++it) {
    theme_combo->Append(*it);
  }
}

void ThemePagePanel::FillFontNameComboBox(wxComboBox* font_combo_box, bool fixed_width_only) {
  FontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, fixed_width_only);
  std::set<wxString>::iterator it = fe.facenames.begin();
  for (; it != fe.facenames.end(); ++it) {
    font_combo_box->Append(*it);
  }
}

void ThemePagePanel::FillFontSizeComboBox(wxComboBox* font_size_combo) {
  for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
    font_size_combo->Append(wxString::Format(wxT("%d"), i));
  }
}

////////////////////////////////////////////////////////////////////////////////

class EditorPagePanel : public wxPanel {
public:
  EditorPagePanel(wxWindow* parent, wxWindowID id = wxID_ANY)
      : wxPanel(parent, id) {
    CreateControls();
  }

private:
  void CreateControls();
};

void EditorPagePanel::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

  //--------------------------------------------------------------------------
  // File type

  {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("File type:"));

    // TODO
    wxComboBox* combo_box = new wxComboBox(this, wxID_ANY, wxEmptyString);
    combo_box->Append(_T("C++"));
    combo_box->Append(_T("Java"));
    combo_box->Append(_T("Python"));
    combo_box->Select(0);

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->AddStretchSpacer();
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->Add(combo_box, wxSizerFlags().Center().Border(wxLEFT));

    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //--------------------------------------------------------------------------
  // Display

  {
    wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Display"));
    wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

    {
      wxCheckBox* check_box = new wxCheckBox(box, wxID_ANY, _("Show horizontal scrollbar"));
      box_vsizer->Add(check_box, wxSizerFlags().Border(wxLTR));
    }

    {
      wxCheckBox* check_box = new wxCheckBox(box, wxID_ANY, _("Show line numbers"));
      box_vsizer->Add(check_box, wxSizerFlags().Border(wxLTR));
    }

    {
      wxCheckBox* check_box = new wxCheckBox(box, wxID_ANY, _("Show white spaces"));
      box_vsizer->Add(check_box, wxSizerFlags().Border(wxLTR));
    }

    {
      wxCheckBox* check_box = new wxCheckBox(box, wxID_ANY, _("Wrap line"));
      box_vsizer->Add(check_box, wxSizerFlags().Border(wxLTR));
    }

    // Rulers
    {
      wxStaticText* label = new wxStaticText(box, wxID_ANY, _("Rulers:"));
      wxTextCtrl* text_ctrl = new wxTextCtrl(box, wxID_ANY, wxEmptyString);

      wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(label, wxSizerFlags().Center());
      hsizer->AddStretchSpacer(1);
      hsizer->Add(text_ctrl, wxSizerFlags().Center().Border(wxLEFT));

      box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxALL));
  }

//--------------------------------------------------------------------------
// Tab

  {
    wxStaticBox* box = new wxStaticBox(this, wxID_ANY, _("Tab"));
    wxSizer* box_vsizer = new wxStaticBoxSizer(box, wxVERTICAL);

    {
      const wxSize kTextCtrlSize(60, -1);

      wxStaticText* ts_label = new wxStaticText(box, wxID_ANY, _("Tab stop:"));
      wxTextCtrl* ts_text_ctrl = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kTextCtrlSize);

      wxStaticText* sw_label = new wxStaticText(box, wxID_ANY, _("Shift width:"));
      wxTextCtrl* sw_text_ctrl = new wxTextCtrl(box, wxID_ANY, wxEmptyString, wxDefaultPosition, kTextCtrlSize);

      wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
      hsizer->Add(ts_label, wxSizerFlags().Center());
      hsizer->Add(ts_text_ctrl, wxSizerFlags().Center().Border(wxLEFT));
      hsizer->AddStretchSpacer(1);
      hsizer->Add(sw_label, wxSizerFlags().Center().Border(wxLEFT));
      hsizer->Add(sw_text_ctrl, wxSizerFlags().Center().Border(wxLEFT));
      box_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
    }

    {
      wxCheckBox* check_box = new wxCheckBox(box, wxID_ANY, _("Expand tabs"));
      box_vsizer->Add(check_box, wxSizerFlags().Border(wxALL));
    }

    top_vsizer->Add(box_vsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //--------------------------------------------------------------------------

  {
    wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Word delimiters:"));
    wxTextCtrl* text_ctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString);

    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(label, wxSizerFlags().Center());
    hsizer->Add(text_ctrl, wxSizerFlags(1).Center().Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  //--------------------------------------------------------------------------

  SetSizerAndFit(top_vsizer);
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