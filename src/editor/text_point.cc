#include "editor/text_point.h"

namespace editor {

bool operator<(const TextPoint& lhs, const TextPoint& rhs) {
  if (lhs.y < rhs.y) {
    return true;
  }
  if (lhs.y == rhs.y) {
    if (rhs.x == kInvCoord) {
      return lhs.x != kInvCoord;
    } else {
      return lhs.x < rhs.x;
    }
  }
  return false;
}

}  // namespace editor
