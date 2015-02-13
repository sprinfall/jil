#ifndef JIL_EDITOR_OPTION_H_
#define JIL_EDITOR_OPTION_H_
#pragma once

#include <map>
#include <string>
#include <vector>
#include "boost/any.hpp"
#include "editor/defs.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

class OptionValue {
public:
  OptionValue() {
  }

  explicit OptionValue(bool data) : data_(data) {
  }

  explicit OptionValue(int data) : data_(data) {
  }

  explicit OptionValue(const std::string& data) : data_(data) {
  }

  bool IsEmpty() const {
    return data_.empty();
  }

  int AsInt() const {
    try {
      return boost::any_cast<int>(data_);
    } catch(const boost::bad_any_cast&) {
      return 0;
    }
  }

  std::string AsString() const {
    try {
      return boost::any_cast<std::string>(data_);
    } catch(const boost::bad_any_cast&) {
      return "";
    }
  }

  bool AsBool() const {
    try {
      return boost::any_cast<bool>(data_);
    } catch(const boost::bad_any_cast&) {
      return false;
    }
  }

private:
  boost::any data_;
};

typedef std::vector<std::pair<std::string, OptionValue> > OptionTable;

////////////////////////////////////////////////////////////////////////////////

// For fast option access (No key-value map).
class Options {
public:
  Options() {
    Init();
  }

  void Init() {
    shift_width = 4;
    tab_stop = 4;
    expand_tab = true;

    comment_add_space = true;
    comment_respect_indent = true;

    wrap = false;
    show_number = true;
    show_space = false;
    show_hscrollbar = false;
  }

  //----------------------------------------------------------------------------
  // Text options

  // The number of spaces to increase indent.
  int shift_width;

  // The number of spaces a tab occupies.
  int tab_stop;

  // Insert spaces for tab.
  bool expand_tab;

  // Operator symbols.
  // E.g., !@#%^&*()+-=\|/?[]{}<>,.;:'"`~
  std::wstring operators;

  // Delimiters determine when a word should end.
  // Normally, an operator is also a delimiter. But delimiters also include
  // space and tab.
  // The difference between operator and delimiter:
  // Take C++ as example, '#' is not an operator, but it's a delimiter.
  // '_' is not an operator, and it is or is not a delimiter.
  // ' ' and '\t' is delimiters, but they are not operators.
  std::wstring delimiters;

  // A list of keys that, when typed, cause reindenting of the current line.
  // Indent keys normally appear at the beginning of a line.
  // Similar to Vim option "indentkeys".
  // E.g., '}' for C/C++, "endif" for Vim Script, etc.
  std::vector<std::wstring> indent_keys;

  // Extra indent options.
  // E.g., indent_namespace, indent_case for C++.
  OptionTable indent_options;

  // Comment options.
  bool comment_add_space;
  bool comment_respect_indent;

  //----------------------------------------------------------------------------
  // View options

  // Wrap line.
  bool wrap;

  // Show line numbers.
  bool show_number;

  // Show spaces and tabs.
  bool show_space;

  // Always show horizontal scroll bar.
  bool show_hscrollbar;

  // Display vertical lines at the given columns.
  std::vector<int> rulers;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_OPTION_H_
