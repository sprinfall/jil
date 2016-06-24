#ifndef JIL_PREF_GLOBAL_FONT_PAGE_H_
#define JIL_PREF_GLOBAL_FONT_PAGE_H_
#pragma once

#include <set>
#include "wx/panel.h"
#include "app/defs.h"  // FONT_COUNT

class wxButton;
class wxCheckBox;
class wxComboBox;
class wxListEvent;

namespace jil {

class Options;

namespace ui {
class StringListCtrl;
}  // namespace ui

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
  void InitFonts();
  void CreateControls();
  void CreateTypeSection(wxSizer* top_vsizer);
  void CreateFontSection(wxSizer* top_vsizer);

  FontType GetSelectedFontType() const;

  void InitNameComboBox(wxComboBox* combo_box, bool fixed_width_only);
  void InitSizeComboBox(wxComboBox* combo_box);

  void SetFontToWindow(const wxFont& font);

  void OnFontListSelectionChange(wxListEvent& evt);

  void OnNameComboBox(wxCommandEvent& evt);
  void OnSizeComboBox(wxCommandEvent& evt);
  void OnFixedWidthOnlyCheckBox(wxCommandEvent& evt);
  void OnUseDefaultButton(wxCommandEvent& evt);

private:
  Options* options_;

  wxFont fonts_[FONT_COUNT];

  ui::StringListCtrl* font_list_ctrl_;

  wxComboBox* name_combo_box_;
  wxComboBox* size_combo_box_;

  wxCheckBox* fixed_width_check_box_;

  wxButton* use_default_button_;

  // TODO: Cache the font list.
  std::set<wxString> font_facenames_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_FONT_PAGE_H_
