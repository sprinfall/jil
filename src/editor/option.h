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
  enum Type {
    kNone = 0,
    kBool,
    kInt,
    kString,
  };

public:
  OptionValue() : type_(kNone) {
  }

  explicit OptionValue(bool data) : data_(data), type_(kBool) {
  }

  explicit OptionValue(int data) : data_(data), type_(kInt) {
  }

  explicit OptionValue(const std::string& data) : data_(data), type_(kString) {
  }

  Type type() const {
    return type_;
  }

  bool IsEmpty() const {
    return data_.empty();
  }

  template <typename T>
  bool As(T* value) const {
    try {
      *value = boost::any_cast<T>(data_);
      return true;
    } catch (const boost::bad_any_cast&) {
      return false;
    }
  }

  wxString ToString() const;

private:
  boost::any data_;
  Type type_;
};

class OptionPair {
public:
  std::string key;
  OptionValue value;
};

typedef std::vector<OptionPair> OptionTable;

////////////////////////////////////////////////////////////////////////////////

// Tab stop range.
const int kMinTabStop = 1;
const int kMaxTabStop = 8;

class TextOptions {
public:
  TextOptions() {
    Init();
  }

  void Init() {
    shift_width = 4;
    tab_stop = 4;
    expand_tab = true;
    guess_tab = true;

    comment_add_space = true;
    comment_respect_indent = true;
  }

  // The number of spaces to increase indent.
  int shift_width;

  // The number of spaces a tab occupies.
  int tab_stop;

  // Expand tab with spaces.
  bool expand_tab;

  // Guess tab options from existing lines.
  bool guess_tab;

  // Word delimiters (exluding ' ' and '\t').
  // E.g., !@#%^$&*()+-=\|/?[]{}<>,.;:'"`~
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
};

class ViewOptions {
public:
  ViewOptions() {
    Init();
  }

  void Init() {
    min_font_size = 0;
    max_font_size = 0;

    wrap = false;
    show_number = true;
    show_space = false;
    show_hscrollbar = false;
  }

  // Font size range.
  // Used in text window when increase/decrease font size with mouse wheel.
  int min_font_size;
  int max_font_size;

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

class Options {
public:
  TextOptions text;
  ViewOptions view;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_OPTION_H_
