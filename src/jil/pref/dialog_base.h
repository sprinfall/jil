#ifndef JIL_PREF_DIALOG_BASE_H_
#define JIL_PREF_DIALOG_BASE_H_

#include "wx/dialog.h"

class wxNotebook;

namespace jil {
namespace pref {

class DialogBase : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  virtual ~DialogBase() = default;

  bool Create(wxWindow* parent, wxWindowID id, const wxString& title);

protected:
  DialogBase() : notebook_(NULL) {
  }

  virtual void AddPages() = 0;

  void OnButtonOK(wxCommandEvent& evt);
  void OnButtonCancel(wxCommandEvent& evt);

protected:
  wxNotebook* notebook_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_DIALOG_BASE_H_
