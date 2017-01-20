#ifndef JIL_ABOUT_DIALOG_H_
#define JIL_ABOUT_DIALOG_H_
#pragma once

#include "wx/dialog.h"

class wxStaticText;

namespace jil {

class AboutDialog : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  AboutDialog(wxWindow* parent, wxWindowID id = wxID_ANY);
  virtual ~AboutDialog();

private:
  wxStaticText* NewLabel(const wxString& str);
};

}  // namespace jil

#endif  // JIL_ABOUT_DIALOG_H_
