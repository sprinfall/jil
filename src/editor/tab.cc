#include "editor/tab.h"
#include "editor/defs.h"

namespace editor {

void TabbedLineFast(int tab_stop, std::wstring* line) {
  std::size_t tabbed_length = 0;
  for (std::size_t x = 0; x < line->size(); ++x) {
    if ((*line)[x] == kTabChar) {
      tabbed_length += tab_stop - (tabbed_length % tab_stop);
      (*line)[x] = kSpaceChar;
    } else {
      ++tabbed_length;
    }
  }

  // Append the extra expanded tab spaces at the end of the line.
  // This is more efficient.
  std::size_t extra_size = tabbed_length - line->size();
  if (extra_size > 0) {
    *line += std::wstring(extra_size, kSpaceChar);
  }
}

Coord TabbedLineLength(int tab_stop, const std::wstring& line, Coord count) {
  if (count == kInvCoord) {
    count = CoordCast(line.length());
  }

  Coord tabbed_length = 0;

  for (Coord x = 0; x < count; ++x) {
    if (line[x] == kTabChar) {
      tabbed_length += tab_stop - (tabbed_length % tab_stop);
    } else {
      ++tabbed_length;
    }
  }

  return tabbed_length;
}

}  // namespace editor
