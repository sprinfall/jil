#ifndef JIL_PREF_GLOBAL_DIALOG_H_
#define JIL_PREF_GLOBAL_DIALOG_H_
#pragma once

#include "jil/pref/dialog_base.h"

class wxBookCtrlEvent;

namespace jil {

class Options;

namespace pref {

// Global preferences dialog.
class GlobalDialog : public DialogBase {
  DECLARE_EVENT_TABLE()

public:
  explicit GlobalDialog(Options* options);
  virtual ~GlobalDialog();

protected:
  virtual void AddPages() override;

  void OnNotebookPageChanged(wxBookCtrlEvent& evt);

private:
  Options* options_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_DIALOG_H_
