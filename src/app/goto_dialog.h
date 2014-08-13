#ifndef JIL_GOTO_DIALOG_H_
#define JIL_GOTO_DIALOG_H_
#pragma once

#include "wx/dialog.h"

class wxTextCtrl;

namespace jil {

// GotoDialog is used for the user to input a line number to go to.
class GotoDialog : public wxDialog {
 public:
  GotoDialog(wxWindow* parent, wxWindowID id);
  virtual ~GotoDialog();

  int line() const {
    return line_;
  }

 private:
  wxTextCtrl* line_text_ctrl_;
  int line_;
};

}  // namespace jil

#endif  // JIL_GOTO_DIALOG_H_
