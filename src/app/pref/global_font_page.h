#ifndef JIL_PREF_GLOBAL_FONT_PAGE_H_
#define JIL_PREF_GLOBAL_FONT_PAGE_H_
#pragma once

#include <map>
#include "wx/panel.h"
#include "wx/odcombo.h"
#include "app/defs.h"  // FONT_COUNT

class wxButton;
class wxComboBox;
class wxListEvent;

namespace jil {

class Options;

namespace ui {
class BoldItemComboBox;
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

  void InitNameComboBox(ui::BoldItemComboBox* combo_box);
  void InitSizeComboBox(wxComboBox* combo_box);

  void SetFontToWindow(const wxFont& font);

  void OnFontListSelectionChange(wxListEvent& evt);

  void OnNameComboBox(wxCommandEvent& evt);
  void OnSizeComboBox(wxCommandEvent& evt);
  void OnUseDefaultButton(wxCommandEvent& evt);

private:
  Options* options_;

  wxFont fonts_[FONT_COUNT];

  ui::StringListCtrl* font_list_ctrl_;

  ui::BoldItemComboBox* name_combo_box_;
  wxComboBox* size_combo_box_;

  wxButton* use_default_button_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_FONT_PAGE_H_
