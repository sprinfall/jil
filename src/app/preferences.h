#ifndef JIL_PREFERENCES_H_
#define JIL_PREFERENCES_H_
#pragma once

#include "wx/intl.h"
#include "wx/dialog.h"

class wxNotebook;

namespace jil {

class Options;

namespace editor {
class Options;
}

////////////////////////////////////////////////////////////////////////////////

class PrefDialogBase : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  virtual ~PrefDialogBase();

  bool Create(wxWindow* parent, wxWindowID id, const wxString& title);

protected:
  PrefDialogBase();

  virtual void AddPages() = 0;

  void OnButtonOK(wxCommandEvent& evt);
  void OnButtonCancel(wxCommandEvent& evt);

protected:
  wxNotebook* notebook_;
};

////////////////////////////////////////////////////////////////////////////////

// Global preferences dialog.
class PrefGlobalDialog : public PrefDialogBase {
public:
  explicit PrefGlobalDialog(Options* options);
  virtual ~PrefGlobalDialog();

protected:
  virtual void AddPages() override;

  wxWindow* CreateGeneralPage();
  wxWindow* CreateFontPage();

private:
  Options* options_;
};

////////////////////////////////////////////////////////////////////////////////

// Editor (Syntax specific) preferences dialog.
class PrefEditorDialog : public PrefDialogBase {
public:
  PrefEditorDialog(editor::Options* options);
  virtual ~PrefEditorDialog();

protected:
  virtual void AddPages() override;

  wxWindow* CreateGeneralPage();
  wxWindow* CreateIndentPage();
  wxWindow* CreateCommentPage();

private:
  editor::Options* options_;
};

}  // namespace jil

#endif  // JIL_PREFERENCES_H_
