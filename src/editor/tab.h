#ifndef EDITOR_TAB_H_
#define EDITOR_TAB_H_

#include <string>

#include "editor/text_point.h"

namespace editor {

// Fast version of tab expanding.
// Suppose tab stop is 4, given "a\tbc" you will get "a bc  ".
void TabbedLineFast(int tab_stop, std::wstring* line);

// Return the tabbed length of line.substr(0, count).
Coord TabbedLineLength(int tab_stop, const std::wstring& line,
                       Coord count = kInvCoord);

}  // namespace editor

#endif  // EDITOR_TAB_H_
