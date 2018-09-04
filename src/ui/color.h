#ifndef UI_COLOR_H_
#define UI_COLOR_H_

#include "wx/colour.h"

namespace ui {

struct Color {
  static wxColour Get(const wxChar* const name);

  // Standard colour names.
  // It's a shame that wxWidgets doesn't provide these constant names.
  static const wxChar* const AQUAMARINE;
  static const wxChar* const BLACK;
  static const wxChar* const BLUE;
  static const wxChar* const BLUE_VIOLET;
  static const wxChar* const BROWN;
  static const wxChar* const CADET_BLUE;
  static const wxChar* const CORAL;
  static const wxChar* const CORNFLOWER_BLUE;
  static const wxChar* const CYAN;
  static const wxChar* const DARK_GREY;
  static const wxChar* const DARK_GREEN;
  static const wxChar* const DARK_OLIVE_GREEN;
  static const wxChar* const DARK_ORCHID;
  static const wxChar* const DARK_SLATE_BLUE;
  static const wxChar* const DARK_SLATE_GREY;
  static const wxChar* const DARK_TURQUOISE;
  static const wxChar* const DIM_GREY;
  static const wxChar* const FIREBRICK;
  static const wxChar* const FOREST_GREEN;
  static const wxChar* const GOLD;
  static const wxChar* const GOLDENROD;
  static const wxChar* const GREY;
  static const wxChar* const GREEN;
  static const wxChar* const GREEN_YELLOW;
  static const wxChar* const INDIAN_RED;
  static const wxChar* const KHAKI;
  static const wxChar* const LIGHT_BLUE;
  static const wxChar* const LIGHT_GREY;
  static const wxChar* const LIGHT_STEEL_BLUE;
  static const wxChar* const LIME_GREEN;
  static const wxChar* const MAGENTA;
  static const wxChar* const MAROON;
  static const wxChar* const MEDIUM_AQUAMARINE;
  static const wxChar* const MEDIUM_BLUE;
  static const wxChar* const MEDIUM_FOREST_GREEN;
  static const wxChar* const MEDIUM_GOLDENROD;
  static const wxChar* const MEDIUM_ORCHID;
  static const wxChar* const MEDIUM_SEA_GREEN;
  static const wxChar* const MEDIUM_SLATE_BLUE;
  static const wxChar* const MEDIUM_SPRING_GREEN;
  static const wxChar* const MEDIUM_TURQUOISE;
  static const wxChar* const MEDIUM_VIOLET_RED;
  static const wxChar* const MIDNIGHT_BLUE;
  static const wxChar* const NAVY;
  static const wxChar* const ORANGE;
  static const wxChar* const ORANGE_RED;
  static const wxChar* const ORCHID;
  static const wxChar* const PALE_GREEN;
  static const wxChar* const PINK;
  static const wxChar* const PLUM;
  static const wxChar* const PURPLE;
  static const wxChar* const RED;
  static const wxChar* const SALMON;
  static const wxChar* const SEA_GREEN;
  static const wxChar* const SIENNA;
  static const wxChar* const SKY_BLUE;
  static const wxChar* const SLATE_BLUE;
  static const wxChar* const SPRING_GREEN;
  static const wxChar* const STEEL_BLUE;
  static const wxChar* const TAN;
  static const wxChar* const THISTLE;
  static const wxChar* const TURQUOISE;
  static const wxChar* const VIOLET;
  static const wxChar* const VIOLET_RED;
  static const wxChar* const WHEAT;
  static const wxChar* const WHITE;
  static const wxChar* const YELLOW;
  static const wxChar* const YELLOW_GREEN;
};

// Example: IncColor(wxColour(125, 125, 125), 10) -> wxColour(135, 135, 135)
wxColour IncColor(const wxColour& from, unsigned char delta);

// Example: DecColor(wxColour(125, 125, 125), 1) -> wxColour(124, 124, 124)
wxColour DecColor(const wxColour& from, unsigned char delta);

}  // namespace ui

#endif  // UI_COLOR_H_
