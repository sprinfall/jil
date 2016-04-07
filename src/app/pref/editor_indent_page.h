#ifndef JIL_PREF_EDITOR_INDENT_PAGE_H_
#define JIL_PREF_EDITOR_INDENT_PAGE_H_
#pragma once

#include "wx/panel.h"
#include <vector>

class wxCheckBox;
class wxTextCtrl;

namespace jil {

namespace editor {
class Options;
}  // namespace editor

class OptionListCtrl;

namespace pref {

class Editor_IndentPage : public wxPanel {
public:
  explicit Editor_IndentPage(editor::Options* options);
  virtual ~Editor_IndentPage();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

protected:
  void CreateControls();

  void CreateTabsSection(wxSizer* top_vsizer);

  void CreateDynamicSection(wxSizer* top_vsizer);

private:
  editor::Options* options_;

  wxTextCtrl* tab_stop_text_ctrl_;
  wxCheckBox* expand_tab_check_box_;
  wxCheckBox* guess_check_box_;
  wxTextCtrl* indent_keys_text_ctrl_;

  OptionListCtrl* option_list_ctrl_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_EDITOR_INDENT_PAGE_H_
