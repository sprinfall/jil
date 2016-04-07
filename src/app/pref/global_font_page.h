#ifndef JIL_PREF_GLOBAL_FONT_PAGE_H_
#define JIL_PREF_GLOBAL_FONT_PAGE_H_
#pragma once

#include "wx/panel.h"
#include "app/defs.h"  // FONT_COUNT

class wxCheckBox;
class wxComboBox;

namespace jil {

class Options;

namespace pref {

class Global_FontPage : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  Global_FontPage(Options* options);
  virtual ~Global_FontPage();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

protected:
  void CreateControls();

  void CreateTypeSection(wxSizer* top_vsizer);

  void CreateFontSection(wxSizer* top_vsizer);

  void InitNameComboBox(wxComboBox* combo_box, bool fixed_width_only);

  void InitSizeComboBox(wxComboBox* combo_box);

  void SetFontToWindow(const wxFont& font);

  void TransferFontToWindow();

  bool TransferFontFromWindow();

  void OnTypeComboBox(wxCommandEvent& evt);

  void OnNameComboBox(wxCommandEvent& evt);

  void OnSizeComboBox(wxCommandEvent& evt);

  void OnFixedWidthOnlyCheckBox(wxCommandEvent& evt);

  void OnResetButton(wxCommandEvent& evt);

private:
  Options* options_;

  wxFont fonts_[FONT_COUNT];

  wxComboBox* type_combo_box_;

  wxComboBox* name_combo_box_;
  wxComboBox* size_combo_box_;

  wxCheckBox* fixed_width_check_box_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_FONT_PAGE_H_
