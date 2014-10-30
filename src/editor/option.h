#ifndef JIL_EDITOR_OPTION_H_
#define JIL_EDITOR_OPTION_H_
#pragma once

#include <string>
#include <vector>
#include "editor/defs.h"

namespace jil {
namespace editor {

// For fast option access (No key-value map).
class Options {
 public:
  Options()
      : wrap(false)
      , shift_width(4)
      , tab_stop(4)
      , expand_tab(true)
      , show_number(true)
      , show_space(false)
      , show_hscrollbar(false) {
  }

  bool wrap;
  int shift_width;
  int tab_stop;
  bool expand_tab;
  bool show_number;
  bool show_space;
  bool show_hscrollbar;

  std::vector<int> rulers;

  // A list of keys that, when typed, cause reindenting of the current line.
  // Indent keys normally appear at the beginning of a line.
  // Similar to Vim option "indentkeys".
  // Examples: '}' for C/C++, "endif" for Vim Script, etc.
  std::vector<std::wstring> indent_keys;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_OPTION_H_
