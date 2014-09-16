#include "editor/selection.h"

namespace jil {
namespace editor {

CharRange Selection::GetCharRange(Coord ln) const {
  if (!rect) {
    return range.GetCharRange(ln);
  }

  Coord x1 = begin().x;
  Coord x2 = end().x;
  if (x1 > x2) {
    std::swap(x1, x2);
  }
  return CharRange(x1, x2);
}

}  // namespace editor
}  // namespace jil
