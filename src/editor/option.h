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

  // Wrap line.
  bool wrap;

  // The number of spaces to increase indent.
  // TODO: Just use tab_stop?
  int shift_width;

  // The number of spaces a tab occupies.
  int tab_stop;

  // Insert spaces for tab.
  bool expand_tab;

  // Show line number area.
  bool show_number;

  // Show spaces or tabs.
  bool show_space;

  // Always show horizontal scroll bar.
  bool show_hscrollbar;

  // Example: !@#%^&*()+-=\|/?[]{}<>,.;:'"`~
  std::wstring operators;

  // Delimiters determine when a word should end.
  // Normally, an operator is also a delimiter. But delimiters also include
  // space and tab.
  // The difference between operator and delimiter:
  // Take C++ as example, '#' is not an operator, but it's a delimiter.
  // '_' is not an operator, and it is or is not a delimiter.
  // ' ' and '\t' is delimiters, but they are not operators.
  std::wstring delimiters;

  // Display vertical lines at the given columns.
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
