#ifndef EDITOR_TEXT_POINT_H_
#define EDITOR_TEXT_POINT_H_

#include "wx/string.h"

#include "editor/defs.h"

namespace editor {

class TextPoint {
public:
  explicit TextPoint(Coord x_ = -1, Coord y_ = 0)
      : x(x_), y(y_) {
  }

  void Set(Coord x_, Coord y_) {
    x = x_;
    y = y_;
  }

  // Reset to an invalid text point.
  void Reset() {
    x = -1;
    y = 0;
  }

  bool Valid() const {
    return x >= 0 && y > 0;
  }

  wxString ToString() const {
    return wxString::Format(wxT("(%d, %d)"), x, y);
  }

  Coord x;  // 0-based
  Coord y;  // 1-based
};

inline bool operator==(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(const TextPoint& lhs, const TextPoint& rhs) {
  return !(lhs == rhs);
}

bool operator<(const TextPoint& lhs, const TextPoint& rhs);

inline bool operator>(const TextPoint& lhs, const TextPoint& rhs) {
  return !(lhs < rhs) && lhs != rhs;
}

inline bool operator<=(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs < rhs || lhs == rhs;
}

inline bool operator>=(const TextPoint& lhs, const TextPoint& rhs) {
  return lhs > rhs || lhs == rhs;
}

inline TextPoint operator+(const TextPoint& lhs, const TextPoint& rhs) {
  return TextPoint(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline TextPoint operator-(const TextPoint& lhs, const TextPoint& rhs) {
  return TextPoint(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline TextPoint& operator+=(TextPoint& lhs, const TextPoint& rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  return lhs;
}

}  // namespace editor

#endif  // EDITOR_TEXT_POINT_H_
