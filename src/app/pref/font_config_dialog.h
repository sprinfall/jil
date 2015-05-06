#ifndef JIL_PREF_FONT_CONFIG_DIALOG_H_
#define JIL_PREF_FONT_CONFIG_DIALOG_H_
#pragma once

#include "wx/dialog.h"

class wxCheckBox;
class wxComboBox;
class wxTextCtrl;

namespace jil {

namespace pref {

class FontConfigDialog : public wxDialog {
public:
  FontConfigDialog(wxWindow* parent);

protected:
  void CreateControls();

  void InitFontNameComboBox(wxComboBox* combo_box, bool fixed_width_only);
  void InitFontSizeComboBox(wxComboBox* combo_box);

private:
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_FONT_CONFIG_DIALOG_H_
