#ifndef JIL_PREF_GLOBAL_FONT_PAGE_H_
#define JIL_PREF_GLOBAL_FONT_PAGE_H_
#pragma once

#include <set>
#include <vector>
#include "wx/panel.h"
#include "wx/thread.h"
#include "jil/defs.h"
#include "jil/font_util.h"

class wxButton;
class wxComboBox;
class wxListEvent;

namespace ui {
class BoldItemComboBox;
class StringListCtrl;
}  // namespace ui

namespace jil {

class Options;

namespace pref {

class Global_FontPage : public wxPanel, public wxThreadHelper {
  DECLARE_EVENT_TABLE()

public:
  Global_FontPage(Options* options);
  virtual ~Global_FontPage();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

  void EnumerateFonts();

protected:
  virtual wxThread::ExitCode Entry() override;

  void OnThreadUpdate(wxThreadEvent& evt);

  void InitFonts();
  void CreateControls();
  void CreateTypeSection(wxSizer* top_vsizer);
  void CreateFontSection(wxSizer* top_vsizer);

  FontType GetSelectedFontType() const;

  void SetFontToWindow(const wxFont& font);

  void OnFontListSelectionChange(wxListEvent& evt);

  void OnNameComboBox(wxCommandEvent& evt);
  void OnSizeComboBox(wxCommandEvent& evt);
  void OnUseDefaultButton(wxCommandEvent& evt);

private:
  Options* options_;

  wxFont fonts_[FONT_COUNT];

  std::vector<wxString> font_names_;

  ui::StringListCtrl* font_list_ctrl_;

  wxComboBox* name_combo_box_;
  wxComboBox* size_combo_box_;

  wxButton* use_default_button_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_FONT_PAGE_H_
