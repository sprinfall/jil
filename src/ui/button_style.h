#ifndef JIL_UI_BUTTON_STYLE_H_
#define JIL_UI_BUTTON_STYLE_H_
#pragma once

#include "wx/colour.h"
#include "boost/shared_ptr.hpp"

namespace jil {
namespace ui {

class ButtonStyle {
public:
  enum Part {
    BG = 0,
    FG,
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

private:
  wxColor colors_[PARTS][STATES];
};

typedef boost::shared_ptr<ButtonStyle> SharedButtonStyle;

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BUTTON_STYLE_H_