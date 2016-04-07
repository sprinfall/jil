#ifndef JIL_PREF_EDITOR_COMMENT_PAGE_H_
#define JIL_PREF_EDITOR_COMMENT_PAGE_H_
#pragma once

#include "wx/panel.h"
#include <vector>

class wxCheckBox;

namespace jil {

namespace editor {
class Options;
}  // namespace editor

namespace pref {

class Editor_CommentPage : public wxPanel {
public:
  explicit Editor_CommentPage(editor::Options* options);
  virtual ~Editor_CommentPage();

  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY);

  virtual bool TransferDataToWindow() override;
  virtual bool TransferDataFromWindow() override;

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
