#ifndef JIL_PREF_GLOBAL_GENERAL_PAGE_H_
#define JIL_PREF_GLOBAL_GENERAL_PAGE_H_

#include <vector>

#include "wx/panel.h"

#include "editor/defs.h"

class wxCheckBox;
class wxComboBox;
class wxSpinCtrl;

namespace jil {

class Options;

namespace pref {

class Global_GeneralPage : public wxPanel {
public:
  explicit Global_GeneralPage(Options* options);

  ~Global_GeneralPage() override = default;

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;

private:
  void CreateControls();

  void CreateEncodingSection(wxSizer* top_vsizer);

  void CreateLinePaddingSection(wxSizer* top_vsizer);

  void InitFileEncodingTable();

  void InitFileEncodingComboBox(wxComboBox* combo_box);

  int IndexFileEncoding(editor::Encoding& encoding) const;

private:
  Options* options_;

  std::vector<editor::Encoding> file_encodings_;

  wxCheckBox* c_check_box_;
  wxCheckBox* j_check_box_;
  wxCheckBox* k_check_box_;

  wxComboBox* fenc_combo_box_;

  wxCheckBox* switch_cwd_check_box_;
  wxCheckBox* restore_files_check_box_;
  wxCheckBox* show_path_check_box_;

  wxSpinCtrl* line_padding_spin_ctrl_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_GENERAL_PAGE_H_
