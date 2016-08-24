#ifndef JIL_PREF_EDITOR_DIALOG_H_
#define JIL_PREF_EDITOR_DIALOG_H_
#pragma once

#include "app/pref/dialog_base.h"

namespace jil {

namespace editor {
class Options;
}  // namespace editor

namespace pref {

// Editor (Syntax specific) preferences dialog.
class EditorDialog : public DialogBase {
public:
  EditorDialog(editor::Options* options);
  virtual ~EditorDialog();

protected:
  virtual void AddPages() override;

private:
  editor::Options* options_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_EDITOR_DIALOG_H_
