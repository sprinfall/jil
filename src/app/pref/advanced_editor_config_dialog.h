#ifndef JIL_PREF_ADVANCED_EDITOR_CONFIG_DIALOG_H_
#define JIL_PREF_ADVANCED_EDITOR_CONFIG_DIALOG_H_
#pragma once

#include "wx/dialog.h"

class wxCheckBox;
class wxTextCtrl;

namespace jil {

namespace pref {

class AdvancedEditorConfigDialog : public wxDialog {
public:
  AdvancedEditorConfigDialog(wxWindow* parent);

protected:
  void CreateControls();

private:
  wxCheckBox* add_space_check_box_;
  wxCheckBox* respect_indent_check_box_;

  wxTextCtrl* sw_text_ctrl_;
  wxTextCtrl* indent_keys_text_ctrl_;
};

}  // namespace jil

}  // namespace pref

#endif  // JIL_PREF_ADVANCED_EDITOR_CONFIG_DIALOG_H_