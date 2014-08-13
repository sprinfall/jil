#include "editor/color.h"
#include "wx/gdicmn.h"

namespace jil {
namespace editor {

wxColour Color::Get(const wxChar* const name) {
  return wxTheColourDatabase->Find(name);
}

const wxChar* const Color::AQUAMARINE = _T("AQUAMARINE");
const wxChar* const Color::BLACK = _T("BLACK");
const wxChar* const Color::BLUE = _T("BLUE");
const wxChar* const Color::BLUE_VIOLET = _T("BLUE VIOLET");
const wxChar* const Color::BROWN = _T("BROWN");
const wxChar* const Color::CADET_BLUE = _T("CADET BLUE");
const wxChar* const Color::CORAL = _T("CORAL");
const wxChar* const Color::CORNFLOWER_BLUE = _T("CORNFLOWER BLUE");
const wxChar* const Color::CYAN = _T("CYAN");
const wxChar* const Color::DARK_GREY = _T("DARK GREY");
const wxChar* const Color::DARK_GREEN = _T("DARK GREEN");
const wxChar* const Color::DARK_OLIVE_GREEN = _T("DARK OLIVE GREEN");
const wxChar* const Color::DARK_ORCHID = _T("DARK ORCHID");
const wxChar* const Color::DARK_SLATE_BLUE = _T("DARK SLATE BLUE");
const wxChar* const Color::DARK_SLATE_GREY = _T("DARK SLATE GREY");
const wxChar* const Color::DARK_TURQUOISE = _T("DARK TURQUOISE");
const wxChar* const Color::DIM_GREY = _T("DIM GREY");
const wxChar* const Color::FIREBRICK = _T("FIREBRICK");
const wxChar* const Color::FOREST_GREEN = _T("FOREST GREEN");
const wxChar* const Color::GOLD = _T("GOLD");
const wxChar* const Color::GOLDENROD = _T("GOLDENROD");
const wxChar* const Color::GREY = _T("GREY");
const wxChar* const Color::GREEN = _T("GREEN");
const wxChar* const Color::GREEN_YELLOW = _T("GREEN YELLOW");
const wxChar* const Color::INDIAN_RED = _T("INDIAN RED");
const wxChar* const Color::KHAKI = _T("KHAKI");
const wxChar* const Color::LIGHT_BLUE = _T("LIGHT BLUE");
const wxChar* const Color::LIGHT_GREY = _T("LIGHT GREY");
const wxChar* const Color::LIGHT_STEEL_BLUE = _T("LIGHT STEEL BLUE");
const wxChar* const Color::LIME_GREEN = _T("LIME GREEN");
const wxChar* const Color::MAGENTA = _T("MAGENTA");
const wxChar* const Color::MAROON = _T("MAROON");
const wxChar* const Color::MEDIUM_AQUAMARINE = _T("MEDIUM AQUAMARINE");
const wxChar* const Color::MEDIUM_BLUE = _T("MEDIUM BLUE");
const wxChar* const Color::MEDIUM_FOREST_GREEN = _T("MEDIUM FOREST GREEN");
const wxChar* const Color::MEDIUM_GOLDENROD = _T("MEDIUM GOLDENROD");
const wxChar* const Color::MEDIUM_ORCHID = _T("MEDIUM ORCHID");
const wxChar* const Color::MEDIUM_SEA_GREEN = _T("MEDIUM SEA GREEN");
const wxChar* const Color::MEDIUM_SLATE_BLUE = _T("MEDIUM SLATE BLUE");
const wxChar* const Color::MEDIUM_SPRING_GREEN = _T("MEDIUM SPRING GREEN");
const wxChar* const Color::MEDIUM_TURQUOISE = _T("MEDIUM TURQUOISE");
const wxChar* const Color::MEDIUM_VIOLET_RED = _T("MEDIUM VIOLET RED");
const wxChar* const Color::MIDNIGHT_BLUE = _T("MIDNIGHT BLUE");
const wxChar* const Color::NAVY = _T("NAVY");
const wxChar* const Color::ORANGE = _T("ORANGE");
const wxChar* const Color::ORANGE_RED = _T("ORANGE RED");
const wxChar* const Color::ORCHID = _T("ORCHID");
const wxChar* const Color::PALE_GREEN = _T("PALE GREEN");
const wxChar* const Color::PINK = _T("PINK");
const wxChar* const Color::PLUM = _T("PLUM");
const wxChar* const Color::PURPLE = _T("PURPLE");
const wxChar* const Color::RED = _T("RED");
const wxChar* const Color::SALMON = _T("SALMON");
const wxChar* const Color::SEA_GREEN = _T("SEA GREEN");
const wxChar* const Color::SIENNA = _T("SIENNA");
const wxChar* const Color::SKY_BLUE = _T("SKY BLUE");
const wxChar* const Color::SLATE_BLUE = _T("SLATE BLUE");
const wxChar* const Color::SPRING_GREEN = _T("SPRING GREEN");
const wxChar* const Color::STEEL_BLUE = _T("STEEL BLUE");
const wxChar* const Color::TAN = _T("TAN");
const wxChar* const Color::THISTLE = _T("THISTLE");
const wxChar* const Color::TURQUOISE = _T("TURQUOISE");
const wxChar* const Color::VIOLET = _T("VIOLET");
const wxChar* const Color::VIOLET_RED = _T("VIOLET RED");
const wxChar* const Color::WHEAT = _T("WHEAT");
const wxChar* const Color::WHITE = _T("WHITE");
const wxChar* const Color::YELLOW = _T("YELLOW");
const wxChar* const Color::YELLOW_GREEN = _T("YELLOW GREEN");

static inline unsigned char IncUChar(unsigned char uc, unsigned char delta) {
  return (uc > 255 - delta) ? 255 : (uc + delta);
}

static inline unsigned char DecUChar(unsigned char uc, unsigned char delta) {
  return (uc < delta) ? 0 : (uc - delta);
}

wxColour IncColor(const wxColour& from, unsigned char delta) {
  return wxColour(IncUChar(from.Red(), delta),
                  IncUChar(from.Green(), delta),
                  IncUChar(from.Blue(), delta));
}

wxColour DecColor(const wxColour& from, unsigned char delta) {
  return wxColour(DecUChar(from.Red(), delta),
                  DecUChar(from.Green(), delta),
                  DecUChar(from.Blue(), delta));
}

}  // namespace editor
}  // namespace jil
