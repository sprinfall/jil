#ifndef JIL_PREFERENCES_H_
#define JIL_PREFERENCES_H_
#pragma once

#include "wx/artprov.h"
#include "wx/intl.h"
#include "wx/preferences.h"
#include "wx/dialog.h"

class wxNotebook;

namespace jil {

class Options;

namespace editor {
class Options;
}

namespace pref {

////////////////////////////////////////////////////////////////////////////////

class Global_GeneralPage : public wxStockPreferencesPage {
public:
  explicit Global_GeneralPage(Options* options)
      : wxStockPreferencesPage(Kind_General)
      , options_(options) {
  }

  virtual wxWindow* CreateWindow(wxWindow* parent) override;

private:
  Options* options_;
};

////////////////////////////////////////////////////////////////////////////////

class Global_FontPage : public wxPreferencesPage {
public:
  explicit Global_FontPage(Options* options) : options_(options) {
  }

  virtual wxString GetName() const override {
    return _("Font");
  }

#ifdef wxHAS_PREF_EDITOR_ICONS
  // TODO
  virtual wxBitmap GetLargeIcon() const override {
    return wxArtProvider::GetBitmap(wxART_INFORMATION);
  }
#endif  // wxHAS_PREF_EDITOR_ICONS

  virtual wxWindow* CreateWindow(wxWindow* parent) override;

private:
  Options* options_;
};

////////////////////////////////////////////////////////////////////////////////

// Editor (Syntax specific) preferences dialog.
class EditorDialog : public wxDialog {
public:
  EditorDialog(editor::Options* options);
  virtual ~EditorDialog();

  bool Create(wxWindow* parent, wxWindowID id, const wxString& title);

private:
  wxWindow* CreateGeneralPage();
  wxWindow* CreateIndentPage();

private:
  editor::Options* options_;

  wxNotebook* notebook_;
};

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREFERENCES_H_
