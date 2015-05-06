#include "app/pref/advanced_editor_config_dialog.h"

#include "wx/checkbox.h"
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"

#include "app/defs.h"

#include "app/pref/defs.h"

namespace jil {

namespace pref {

AdvancedEditorConfigDialog::AdvancedEditorConfigDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Advanced Editor Preferences")) {
  CreateControls();
}

void AdvancedEditorConfigDialog::CreateControls() {
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

}  // namespace jil

}  // namespace pref
