#include "editor/text_point.h"

namespace jil {
namespace editor {

bool operator<(const TextPoint& lhs, const TextPoint& rhs) {
  if (lhs.y < rhs.y) {
    return true;
  }
  if (lhs.y == rhs.y) {
    if (rhs.x == kInvalidCoord) {
      return lhs.x != kInvalidCoord;
    } else {
      return lhs.x < rhs.x;
    }
  }
  return false;
}

}  // namespace editor
}  // namespace jil
