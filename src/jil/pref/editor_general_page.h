#ifndef JIL_PREF_EDITOR_GENERAL_PAGE_H_
#define JIL_PREF_EDITOR_GENERAL_PAGE_H_
#pragma once

#include "wx/panel.h"
#include <vector>

class wxCheckBox;
class wxTextCtrl;

namespace editor {
class Options;
}  // namespace editor

namespace jil {

namespace pref {

class Editor_GeneralPage : public wxPanel {
public:
  explicit Editor_GeneralPage(editor::Options* options);
  virtual ~Editor_GeneralPage();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  wxString JoinRulers(const std::vector<int>& rulers);

  void SplitRulers(const wxString& rulers_str, std::vector<int>& rulers);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

protected:
  void CreateControls();

  void CreateDisplaySection(wxSizer* top_vsizer);

  void CreateRulersSection(wxSizer* top_vsizer);

  void CreateDelimitersSection(wxSizer* top_vsizer);

private:
  editor::Options* options_;

  wxCheckBox* show_hscrollbar_check_box_;
  wxCheckBox* show_number_check_box_;
  wxCheckBox* show_space_check_box_;
  wxCheckBox* wrap_check_box_;
  wxTextCtrl* rulers_text_ctrl_;
  wxTextCtrl* delimiters_text_ctrl_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_EDITOR_GENERAL_PAGE_H_
