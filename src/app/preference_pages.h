#ifndef JIL_PREFERENCE_PAGES_H_
#define JIL_PREFERENCE_PAGES_H_
#pragma once

#include "wx/panel.h"
#include "wx/preferences.h"
#include "wx/artprov.h"

namespace jil {

namespace pref {

class GeneralPage : public wxStockPreferencesPage {
public:
  GeneralPage()
      : wxStockPreferencesPage(Kind_General) {
  }

  virtual wxWindow* CreateWindow(wxWindow* parent) override;
};

class ThemePage : public wxPreferencesPage {
public:
  virtual wxString GetName() const override {
    return _("Theme");
  }

  virtual wxWindow* CreateWindow(wxWindow* parent) override;
};

class EditorPage : public wxPreferencesPage {
public:
  virtual wxString GetName() const override {
    return _("Editor");
  }

  virtual wxWindow* CreateWindow(wxWindow* parent) override;
};

}  // namespace pref

}  // namespace jil

#endif  // JIL_PREFERENCE_PAGES_H_
