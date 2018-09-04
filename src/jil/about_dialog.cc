#include "jil/about_dialog.h"

#include "wx/sizer.h"
#include "wx/stattext.h"

#include "jil/defs.h"

namespace jil {

AboutDialog::AboutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, kAppDisplayName) {
  SetMinSize(wxSize(390, -1));

  // Name
  wxStaticText* name_label = NewLabel(kAppDisplayName);
  wxFont name_font = GetFont();
  name_font.SetPointSize(16);
  name_label->SetFont(name_font);

  // Version
  wxStaticText* version_label = NewLabel(kAppVersion);

  // Author
  wxString author = wxT("Chunting Gu, sprinfall@gmail.com");
  wxStaticText* author_label = NewLabel(author);

  // Years
  // Don't aumatically use the current year! Using the last update year makes
  // more sense.
  wxStaticText* years_label = NewLabel(wxT("2018"));

  // Layout
  wxSizer* top_vsizer = new wxBoxSizer(wxVERTICAL);
  top_vsizer->Add(name_label, 0, wxALIGN_CH | wxLR, 10);
  top_vsizer->Add(version_label, 0, wxALIGN_CH | wxLR, 10);
  top_vsizer->AddSpacer(30);
  top_vsizer->Add(author_label, 0, wxALIGN_CH | wxLR, 10);
  top_vsizer->Add(years_label, 0, wxALIGN_CH | wxLR, 10);
  top_vsizer->AddSpacer(10);

  SetSizer(top_vsizer);
  Fit();
}

AboutDialog::~AboutDialog() {
}

wxStaticText* AboutDialog::NewLabel(const wxString& str) {
  return new wxStaticText(this, wxID_ANY, str);
}

}  // namespace jil
