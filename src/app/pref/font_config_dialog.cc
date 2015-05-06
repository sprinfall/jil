#include "app/pref/font_config_dialog.h"

#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/stattext.h"

#include "app/font_util.h"

#include "app/pref/defs.h"

namespace jil {

namespace pref {

FontConfigDialog::FontConfigDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Fonts")) {
  CreateControls();
}

void FontConfigDialog::CreateControls() {
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);

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

void FontConfigDialog::InitFontNameComboBox(wxComboBox* combo_box, bool fixed_width_only) {
  FontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, fixed_width_only);
  std::set<wxString>::iterator it = fe.facenames.begin();
  for (; it != fe.facenames.end(); ++it) {
    combo_box->Append(*it);
  }
}

void FontConfigDialog::InitFontSizeComboBox(wxComboBox* combo_box) {
  for (int i = kMinFontSize; i <= kMaxFontSize; ++i) {
    combo_box->Append(wxString::Format(wxT("%d"), i));
  }
}

}  // namespace pref
}  // namespace jil
