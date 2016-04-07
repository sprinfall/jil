#include "app/font_util.h"
#include <set>
#include "wx/gdicmn.h"
#include "wx/settings.h"

namespace jil {

const wxFont& GetGlobalFont(int point_size, const wxString& facename, bool bold, bool italic) {
  wxFont* font = wxTheFontList->FindOrCreateFont(
      point_size,
      wxFONTFAMILY_DEFAULT,
      italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
      bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
      false,
      facename);
  assert(font != NULL);
  return *font;
}

// TODO
static wxString DoGetDefaultFontName() {
  OrderedFontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
  if (fe.facenames.empty()) {
    // No fixed-width fonts? Use system font.
    return wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT).GetFaceName();
  }

  // Some preferred fixed-width fonts.
  static const wxString kBestFonts[] = {
#if defined (__WXOSX__)
    wxT("Menlo"),  // Xcode default font.
#endif
    wxT("DejaVu Sans Mono"),
    wxT("Bitstream Vera Sans Mono"),
    wxT("Consolas"),
    wxT("Courier New"),
  };

  size_t count = sizeof(kBestFonts) / sizeof(wxString);
  for (size_t i = 0; i < count; ++i) {
    if (fe.facenames.find(kBestFonts[i]) != fe.facenames.end()) {
      return kBestFonts[i];
    }
  }

  return *(fe.facenames.begin());
}

wxString GetDefaultFontName() {
  static wxString font = DoGetDefaultFontName();
  return font;
}

}  // namespace jil
