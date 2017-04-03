#ifndef JIL_PREF_GLOBAL_THEME_PAGE_H_
#define JIL_PREF_GLOBAL_THEME_PAGE_H_
#pragma once

#include "wx/panel.h"
#include <vector>
#include "editor/defs.h"

class wxCheckBox;
class wxComboBox;
class wxStaticText;

namespace jil {

class Options;

namespace pref {

class Global_ThemePage : public wxPanel {
public:
  Global_ThemePage(Options* options);
  virtual ~Global_ThemePage();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

private:
  void CreateControls();

  void LayoutField(wxSizer* top_vsizer, wxStaticText* label, wxComboBox* combo_box);

private:
  Options* options_;

  wxComboBox* theme_combo_box_;
  wxCheckBox* enlarge_icons_check_box_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_THEME_PAGE_H_
