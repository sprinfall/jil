#ifndef JIL_PREF_EDITOR_CONFIG_PANEL_H_
#define JIL_PREF_EDITOR_CONFIG_PANEL_H_
#pragma once

#include "wx/panel.h"

class wxCheckBox;
class wxComboBox;
class wxTextCtrl;

namespace jil {
namespace pref {

class EditorConfigPanel : public wxPanel {
public:
  EditorConfigPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

protected:
  void CreateControls();

  void InitFtComboBox(wxComboBox* combo_box);

  void OnFtComboBox(wxCommandEvent& evt);

  void OnMoreButtonClick(wxCommandEvent& evt);

private:
  wxComboBox* ft_combo_box_;

  wxCheckBox* show_hscrollbar_check_box_;
  wxCheckBox* show_number_check_box_;
  wxCheckBox* show_space_check_box_;
  wxCheckBox* wrap_check_box_;

  wxTextCtrl* rulers_text_ctrl_;

  wxTextCtrl* delimiters_text_ctrl_;

  wxTextCtrl* ts_text_ctrl_;
  wxCheckBox* expand_tab_check_box_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_EDITOR_CONFIG_PANEL_H_
