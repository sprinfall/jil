#include "ui/button_style.h"

namespace jil {
namespace ui {

void ButtonStyle::Fix() {
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
