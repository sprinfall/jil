#ifndef JIL_PREF_GENERAL_CONFIG_PANEL_H_
#define JIL_PREF_GENERAL_CONFIG_PANEL_H_
#pragma once

#include "wx/panel.h"

class wxButton;
class wxCheckBox;
class wxComboBox;

namespace jil {

class Options;

namespace pref {

class GeneralConfigPanel : public wxPanel {
public:
  GeneralConfigPanel(Options* options);
  virtual ~GeneralConfigPanel();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

private:
  void CreateControls();

  void InitFileEncodingComboBox(wxComboBox* combo_box);

private:
  Options* options_;

  wxCheckBox* c_check_box_;
  wxCheckBox* j_check_box_;
  wxCheckBox* k_check_box_;

  wxComboBox* fenc_combo_box_;

  wxCheckBox* switch_cwd_check_box_;
  wxCheckBox* restore_files_check_box_;
  wxCheckBox* show_path_check_box_;

  wxButton* fonts_button_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GENERAL_CONFIG_PANEL_H_
