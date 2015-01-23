#include "app/goto_dialog.h"
#include "wx/button.h"
#include "wx/sizer.h"
#include "wx/textctrl.h"
#include "wx/valnum.h"

namespace jil {

GotoDialog::GotoDialog(wxWindow* parent, wxWindowID id)
    : wxDialog(parent, id, _("Go To"))
    , line_text_ctrl_(NULL)
    , line_(0) {
  // Allow integer only. (Better to use unsigned int.)
  wxIntegerValidator<int> val(&line_, wxNUM_VAL_DEFAULT);

  line_text_ctrl_ = new wxTextCtrl(this,
                                   wxID_ANY,
                                   wxEmptyString,
                                   wxDefaultPosition,
                                   wxSize(300, -1),
                                   0,
                                   val);

  wxButton* ok_button = new wxButton(this, wxID_OK, _("OK"));
  wxButton* cancel_button = new wxButton(this, wxID_CANCEL, _("Cancel"));

  ok_button->SetDefault();  // Set default for ENTER key.

  const int kSpace = 7;

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->AddSpacer(kSpace);
  vsizer->Add(line_text_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT, kSpace);

  wxSizer* button_hsizer = new wxBoxSizer(wxHORIZONTAL);
  button_hsizer->AddStretchSpacer();
  button_hsizer->Add(ok_button);
  button_hsizer->AddSpacer(kSpace);
  button_hsizer->Add(cancel_button);
  vsizer->AddSpacer(kSpace);
  vsizer->Add(button_hsizer, 0, wxEXPAND | wxLEFT | wxRIGHT, kSpace);

  vsizer->AddSpacer(kSpace);

  SetSizer(vsizer);
  Fit();
}

GotoDialog::~GotoDialog() {
}

}  // namespace jil
