#ifndef JIL_FONT_UTIL_H_
#define JIL_FONT_UTIL_H_

#include <list>
#include <set>

#include "wx/font.h"
#include "wx/fontenum.h"
#include "wx/string.h"

#include "jil/defs.h"

namespace jil {

// Get or create the font via wxTheFontList.
const wxFont& GetGlobalFont(int point_size,
                            const wxString& facename,
                            bool bold = false,
                            bool italic = false);

// Get a default preferred font.
wxString GetDefaultFontName();

// Get the default font point size.
int GetDefaultFontSize();

void NormalizeFont(wxFont& font);

wxFont GetDefaultFont(FontType font_type);

// Font point sizes.
#if defined (__WXOSX__)
const int kMinFontSize = 8;
const int kMaxFontSize = 36;
#else
const int kMinFontSize = 6;
const int kMaxFontSize = 24;
#endif

// Usage:
// OrderedFontEnumerator fe;
// fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
// if (fe.facenames.empty()) {
//   ...
// }
class OrderedFontEnumerator : public wxFontEnumerator {
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
