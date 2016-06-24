#include "app/font_util.h"

#include <set>

#include "wx/gdicmn.h"
#include "wx/log.h"
#include "wx/settings.h"

#include "app/defs.h"

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
  static wxString name = DoGetDefaultFontName();
  return name;
}

// NOTE:
// According to wxGTK's poor implementation of wxSystemSettings::GetFont, you
// cannot get the system fixed-width font under GTK+.
static int DoGetDefaultFontSize() {
  // TODO: Check the font size for wxSYS_SYSTEM_FIXED_FONT in Windows.
  wxFont font = wxSystemSettings::GetFont(wxSYS_SYSTEM_FIXED_FONT);
  return font.GetPointSize();
}

int GetDefaultFontSize() {
  static int size = DoGetDefaultFontSize();
  return size;
}

void NormalizeFont(wxFont& font) {
  if (font.GetWeight() != wxFONTWEIGHT_NORMAL) {
    font.SetWeight(wxFONTWEIGHT_NORMAL);
  }

  if (font.GetStyle() != wxFONTSTYLE_NORMAL) {
    font.SetStyle(wxFONTSTYLE_NORMAL);
  }
}

static void InitDefaultFonts(wxFont fonts[FONT_COUNT]) {
  fonts[FONT_TEXT] = GetGlobalFont(GetDefaultFontSize(), GetDefaultFontName());
  fonts[FONT_LINE_NR] = fonts[FONT_TEXT];

  wxFont gui_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  NormalizeFont(gui_font);

  fonts[FONT_TABS] = gui_font;
  fonts[FONT_STATUS_BAR] = gui_font;
}

wxFont GetDefaultFont(FontType font_type) {
  wxFont fonts[FONT_COUNT];
  if (!fonts[0].IsOk()) {
    InitDefaultFonts(fonts);
  }
  return fonts[font_type];
}

}  // namespace jil
