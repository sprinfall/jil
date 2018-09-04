#ifndef JIL_PREF_EDITOR_DIALOG_H_
#define JIL_PREF_EDITOR_DIALOG_H_

#include "jil/pref/dialog_base.h"

namespace editor {
class Options;
}  // namespace editor

namespace jil {
namespace pref {

// Editor (Syntax specific) preferences dialog.
class EditorDialog : public DialogBase {
public:
  explicit EditorDialog(editor::Options* options) : options_(options) {
  }

  ~EditorDialog() override = default;

protected:
  void AddPages() override;

private:
  editor::Options* options_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_EDITOR_DIALOG_H_
