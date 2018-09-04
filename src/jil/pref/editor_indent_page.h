#ifndef JIL_PREF_EDITOR_INDENT_PAGE_H_
#define JIL_PREF_EDITOR_INDENT_PAGE_H_

#include <vector>

#include "wx/panel.h"

class wxCheckBox;
class wxTextCtrl;

namespace editor {
class Options;
}  // namespace editor

namespace jil {

namespace pref {

class OptionListCtrl;

class Editor_IndentPage : public wxPanel {
public:
  explicit Editor_IndentPage(editor::Options* options) : options_(options) {
  }

  ~Editor_IndentPage() override = default;

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;

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
