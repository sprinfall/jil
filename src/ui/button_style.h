#ifndef JIL_UI_BUTTON_STYLE_H_
#define JIL_UI_BUTTON_STYLE_H_
#pragma once

#include "boost/shared_ptr.hpp"
#include "wx/colour.h"

namespace jil {
namespace ui {

class ButtonStyle {
public:
  enum Part {
    FG = 0,
    BG1,
    BG2,  // Optional, use BG1 if not provided.
    BG3,  // Optional, use BG2 if not provided.
    BORDER_OUTER,
    BORDER_INNER,  // Optional, use BORDER_OUTER if not provided.
    PARTS,
  };

  enum State {
    NORMAL = 0,
    NORMAL_HOVER,
    PRESSED,
    PRESSED_HOVER,
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

typedef boost::shared_ptr<ButtonStyle> SharedButtonStyle;

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BUTTON_STYLE_H_