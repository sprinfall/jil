#include "app/pref/editor_config_panel.h"

#include "wx/button.h"
#include "wx/checkbox.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"

#include "app/defs.h"

#include "app/pref/advanced_editor_config_dialog.h"
#include "app/pref/defs.h"

namespace jil {
namespace pref {

EditorConfigPanel::EditorConfigPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
  CreateControls();
}

void EditorConfigPanel::CreateControls() {
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
  Connect(more_button->GetId(), wxEVT_BUTTON, wxCommandEventHandler(EditorConfigPanel::OnMoreButtonClick));

  SetSizerAndFit(top_vsizer);
}

// TODO
void EditorConfigPanel::InitFtComboBox(wxComboBox* combo_box) {
  combo_box->Append(_T("C++"));
  combo_box->Append(_T("Java"));
  combo_box->Append(_T("Python"));
  combo_box->Select(0);
}

void EditorConfigPanel::OnFtComboBox(wxCommandEvent& evt) {
}

void EditorConfigPanel::OnMoreButtonClick(wxCommandEvent& evt) {
  AdvancedEditorConfigDialog* more_dialog = new AdvancedEditorConfigDialog(this);
  more_dialog->ShowModal();
}

}  // namespace pref
}  // namespace jil
