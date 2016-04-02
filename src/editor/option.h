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
  OptionValue()
      : type_(kNone) {
  }

  OptionValue(const OptionValue& rhs)
      : data_(rhs.data_), type_(rhs.type_) {
  }

  // NOTE:
  // Given overloaded contructors:
  //   OptionValue(bool data);
  //   OptionValue(const std::string& data);
  //   ...
  // OptionValue("") will "incorrectly" invoke the bool version.
  // So don't provide overloaded constructors!

  static OptionValue FromBool(bool data) {
    return OptionValue(data, kBool);
  }

  static OptionValue FromInt(int data) {
    return OptionValue(data, kInt);
  }

  static OptionValue FromString(const std::string& data) {
    return OptionValue(data, kString);
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

  template <typename T>
  T As() const {
    try {
      return boost::any_cast<T>(data_);
    } catch (const boost::bad_any_cast&) {
      return T();
    }
  }

  int AsInt() const {
    return As<int>();
  }

  std::string AsString() const {
    return As<std::string>();
  }

  bool AsBool() const {
    return As<bool>();
  }

  wxString ToString() const;

  bool Parse(const wxString& str);

private:
  OptionValue(const boost::any& data, Type type)
      : data_(data), type_(type) {
  }

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
    tab_stop = 4;
    expand_tab = true;
    guess_tab = true;

    comment_add_space = true;
    comment_respect_indent = true;
  }

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

  // Add a space after the comment leader.
  // Examples (C++):
  // - true:
  //   if (foo) {
  //       // bar();
  //   }
  // - false:
  //   if (foo) {
  //       //bar();
  //   }
  bool comment_add_space;

  // Respect line indent.
  // Examples (C++):
  // - true:
  //   if (foo) {
  //       //bar();
  //   }
  // - false:
  //   if (foo) {
  //   //    bar();
  //   }
  bool comment_respect_indent;
};


class ViewOptions {
public:
  ViewOptions() {
    Init();
  }

  void Init() {
    wrap = false;
    show_number = true;
    show_space = false;
    show_hscrollbar = false;
  }

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

bool operator==(const ViewOptions& lhs, const ViewOptions& rhs);

inline bool operator!=(const ViewOptions& lhs, const ViewOptions& rhs) {
  return !(lhs == rhs);
}

class Options {
public:
  TextOptions text;
  ViewOptions view;
};

class TabOptions {
public:
  TabOptions(int _tab_stop = 0, bool _expand_tab = false)
      : tab_stop(_tab_stop), expand_tab(_expand_tab) {
  }

  int tab_stop;
  bool expand_tab;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_OPTION_H_
