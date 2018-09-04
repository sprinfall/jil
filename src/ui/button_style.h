#ifndef UI_BUTTON_STYLE_H_
#define UI_BUTTON_STYLE_H_

#include <memory>

#include "wx/colour.h"

namespace ui {

class ButtonStyle {
public:
  enum Part {
    FG = 0,
    BG,
    BORDER,
    PARTS,
  };

  enum State {
    NORMAL = 0,
    HOVER,
    PRESSED,
    DISABLED,
    STATES,
  };

public:
  const wxColour& GetColor(int part, int state) const {
    assert(part >= 0 && part < PARTS);
    assert(state >= 0 && state < STATES);
    return colors_[part][state];
  }

  void SetColor(int part, int state, const wxColour& color) {
    assert(part >= 0 && part < PARTS);
    assert(state >= 0 && state < STATES);
    colors_[part][state] = color;
  }

  // Fix missing colors.
  void Fix();

private:
  wxColor colors_[PARTS][STATES];
};

typedef std::shared_ptr<ButtonStyle> SharedButtonStyle;

}  // namespace ui

#endif  // UI_BUTTON_STYLE_H_
