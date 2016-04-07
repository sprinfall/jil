#ifndef JIL_PREF_GLOBAL_DIALOG_H_
#define JIL_PREF_GLOBAL_DIALOG_H_
#pragma once

#include "app/pref/dialog_base.h"

namespace jil {

class Options;

namespace pref {

// Global preferences dialog.
class GlobalDialog : public DialogBase {
public:
  explicit GlobalDialog(Options* options);
  virtual ~GlobalDialog();

protected:
  virtual void AddPages() override;

  wxWindow* CreateGeneralPage();
  wxWindow* CreateFontPage();

private:
  Options* options_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GLOBAL_DIALOG_H_
