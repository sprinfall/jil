#include "jil/pref/dialog_base.h"
#include "wx/notebook.h"
#include "wx/sizer.h"
#include "jil/pref/common.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(DialogBase, wxDialog)
EVT_BUTTON(wxID_OK,       DialogBase::OnButtonOK)
EVT_BUTTON(wxID_CANCEL,   DialogBase::OnButtonCancel)
END_EVENT_TABLE()

DialogBase::DialogBase() : notebook_(NULL) {
}

DialogBase::~DialogBase() {
}

bool DialogBase::Create(wxWindow* parent, wxWindowID id, const wxString& title) {
  // Recursively call TransferDataFromWindow() and TransferDataToWindow().
  SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);

  if (!wxDialog::Create(parent, id, title)) {
    return false;
  }

  notebook_ = new wxNotebook(this, ID_NOTEBOOK);

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

void DialogBase::OnButtonOK(wxCommandEvent& evt) {
  TransferDataFromWindow();
  EndModal(wxID_OK);
}

void DialogBase::OnButtonCancel(wxCommandEvent& evt) {
  EndModal(wxID_CANCEL);
}

}  // namespace pref
}  // namespace jil
