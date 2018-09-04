#ifndef JIL_PREF_EDITOR_COMMENT_PAGE_H_
#define JIL_PREF_EDITOR_COMMENT_PAGE_H_

#include <vector>

#include "wx/panel.h"

class wxCheckBox;

namespace editor {
class Options;
}  // namespace editor

namespace jil {

namespace pref {

class Editor_CommentPage : public wxPanel {
public:
  explicit Editor_CommentPage(editor::Options* options) : options_(options) {
  }

  ~Editor_CommentPage() override = default;

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  bool TransferDataToWindow() override;
  bool TransferDataFromWindow() override;

protected:
  void CreateControls();

private:
  editor::Options* options_;

  wxCheckBox* add_space_check_box_;
  wxCheckBox* respect_indent_check_box_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_EDITOR_COMMENT_PAGE_H_
