#ifndef JIL_PREFERENCE_PAGES_H_
#define JIL_PREFERENCE_PAGES_H_
#pragma once

#include "wx/panel.h"
#include "wx/preferences.h"
#include "wx/artprov.h"

namespace jil {

class Options;

namespace pref {

class GeneralPage : public wxStockPreferencesPage {
public:
  explicit GeneralPage(Options* options)
      : wxStockPreferencesPage(Kind_General)
      , options_(options) {
  }

  virtual wxWindow* CreateWindow(wxWindow* parent) override;

private:
  Options* options_;
};

class ThemePage : public wxPreferencesPage {
public:
  virtual wxString GetName() const override {
    return _("Theme");
  }

#ifdef wxHAS_PREF_EDITOR_ICONS
  // TODO
  virtual wxBitmap GetLargeIcon() const override {
    return wxArtProvider::GetBitmap(wxART_INFORMATION);
  }
#endif  // wxHAS_PREF_EDITOR_ICONS

  virtual wxWindow* CreateWindow(wxWindow* parent) override;
};

class EditorPage : public wxPreferencesPage {
public:
  virtual wxString GetName() const override {
    return _("Editor");
  }

#ifdef wxHAS_PREF_EDITOR_ICONS
  // TODO
  virtual wxBitmap GetLargeIcon() const override {
    return wxArtProvider::GetBitmap(wxART_INFORMATION);
  }
#endif  // wxHAS_PREF_EDITOR_ICONS

  virtual wxWindow* CreateWindow(wxWindow* parent) override;
};

}  // namespace pref

}  // namespace jil

#endif  // JIL_PREFERENCE_PAGES_H_
