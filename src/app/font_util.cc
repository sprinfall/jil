#include "app/font_util.h"
#include <set>
#include "wx/gdicmn.h"
#include "wx/fontenum.h"
#include "wx/settings.h"

namespace jil {

const wxFont& GetGlobalFont(int point_size,
                            const wxString& facename,
                            bool bold,
                            bool italic) {
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

class FontEnumerator : public wxFontEnumerator {
public:
  virtual bool OnFacename(const wxString& facename) override {
    facenames.insert(facename);
    return true;
  }

  std::set<wxString> facenames;
};

// Some preferred fixed-width fonts.
#define kFontDejaVuSansMono _T("DejaVu Sans Mono")
#define kFontBitstreamVeraSansMono _T("Bitstream Vera Sans Mono")
#define kFontCourierNew _T("Courier New")

static wxString DoGetDefaultFont() {
  FontEnumerator fe;
  fe.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
  if (fe.facenames.empty()) {  // No fixed-width fonts.
    // Use system font.
    return wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT).GetFaceName();
  }

  if (fe.facenames.find(kFontDejaVuSansMono) != fe.facenames.end()) {
    return kFontDejaVuSansMono;
  }
  if (fe.facenames.find(kFontBitstreamVeraSansMono) != fe.facenames.end()) {
    return kFontBitstreamVeraSansMono;
  }
  if (fe.facenames.find(kFontCourierNew) != fe.facenames.end()) {
    return kFontCourierNew;
  }
  return *(fe.facenames.begin());
}

wxString GetDefaultFont() {
  static wxString font = DoGetDefaultFont();
  return font;
}

}  // namespace jil
