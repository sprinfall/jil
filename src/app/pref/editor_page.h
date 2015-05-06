#ifndef JIL_PREF_EDITOR_PAGE_H_
#define JIL_PREF_EDITOR_PAGE_H_
#pragma once

#include "wx/artprov.h"
#include "wx/intl.h"
#include "wx/preferences.h"

namespace jil {

class Options;
 
namespace pref {

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

#endif  // JIL_PREF_EDITOR_PAGE_H_
