#ifndef JIL_EDITOR_TAB_H_
#define JIL_EDITOR_TAB_H_
#pragma once

#include <string>
#include "editor/text_point.h"

namespace jil {
namespace editor {

// Fast version of tab expanding.
// Suppose tab stop is 4, given "a\tbc" you will get "a bc  ".
void TabbedLineFast(int tab_stop, std::wstring* line);

// Return the tabbed length of line.substr(0, count).
Coord TabbedLineLength(int tab_stop,
                       const std::wstring& line,
                       Coord count = kInvalidCoord);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TAB_H_
