#ifndef JIL_FONT_UTIL_H_
#define JIL_FONT_UTIL_H_
#pragma once

#include <set>
#include "wx/font.h"
#include "wx/fontenum.h"
#include "wx/string.h"

namespace jil {

// Get or create the font via wxTheFontList.
const wxFont& GetGlobalFont(int point_size, const wxString& facename, bool bold = false, bool italic = false);

// Min and max font point sizes.
const int kMinFontSize = 8;
const int kMaxFontSize = 24;

// Get a default preferred font.
wxString GetDefaultFontName();

int GetDefaultFontSize();

// Usage:
// FontEnumerator fe;
// fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
// if (fe.facenames.empty()) {
//   ...
// }
class FontEnumerator : public wxFontEnumerator {
public:
  virtual bool OnFacename(const wxString& facename) override {
    if (facename[0] != wxT('@')) {
      facenames.insert(facename);
    }
    return true;
  }

  std::set<wxString> facenames;
};

}  // namespace jil

#endif  // JIL_FONT_UTIL_H_
