#ifndef JIL_EDITOR_SELECTION_H_
#define JIL_EDITOR_SELECTION_H_
#pragma once

#include "editor/defs.h"
#include "editor/text_range.h"

namespace jil {
namespace editor {

// Text selection.
class Selection {
public:
  Selection() : dir(kForward), rect(false) {
  }

  void Reset() {
    range.Reset();
    rect = false;
  }

  void Set(const TextRange _range, TextDir _dir, bool _rect) {
    range = _range;
    dir = _dir;
    rect = _rect;
  }

  bool IsEmpty() const {
    return range.IsEmpty();
  }

  bool IsRectEmpty() const {
    return (rect && range.point_begin().x == range.point_end().x);
  }

  bool Contain(const TextPoint& point) const {
    return range.Contain(point);
  }

  bool HasLine(Coord ln) const {
    return range.HasLine(ln);
  }

  const TextPoint& begin() const {
    return range.point_begin();
  }

  const TextPoint& end() const {
    return range.point_end();
  }

  const TextPoint& GetFromPoint() const {
    return (dir == kForward ? begin() : end());
  }

  const TextPoint& GetToPoint() const {
    return (dir == kForward ? end() : begin());
  }

  LineRange GetLineRange() const {
    return range.GetLineRange();
  }

  CharRange GetCharRange(Coord ln) const {
    if (rect) {
      return range.GetCharRange();
    } else {
      return range.GetCharRange(ln);
    }
  }

  CharRange GetCharRange() const {
    return range.GetCharRange();
  }

  TextRange range;
  TextDir dir;
  bool rect;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_SELECTION_H_
