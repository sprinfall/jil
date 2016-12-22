#include "ui/button_style.h"

namespace jil {
namespace ui {

void ButtonStyle::Fix() {
  // If BG2 is not available, use BG1.
  // If BG3 is not available, use BG2.
  for (int i = BG2; i <= BG3; ++i) {
    if (!colors_[i][NORMAL].IsOk()) {
      for (int j = 1; j < STATES; ++j) {
        colors_[i][j] = colors_[i-1][j];
      }
    }
  }

  // If BORDER_INNER is not available, use BORDER_OUTER.
  if (!colors_[BORDER_INNER][NORMAL].IsOk()) {
    for (int j = 1; j < STATES; ++j) {
      colors_[BORDER_INNER][j] = colors_[BORDER_OUTER][j];
    }
  }

  // If any non-NORMAL is not available, use NORMAL.
  for (int i = 0; i < PARTS; ++i) {
    for (int j = 1; j < STATES; ++j) {
      if (!colors_[i][j].IsOk()) {
        colors_[i][j] = colors_[i][j-1];
      }
    }
  }
}

}  // namespace ui
}  // namespace jil
