#ifndef JIL_PREF_GENERAL_PAGE_H_
#define JIL_PREF_GENERAL_PAGE_H_
#pragma once

#include "wx/artprov.h"
#include "wx/preferences.h"

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

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_GENERAL_PAGE_H_
