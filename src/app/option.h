#ifndef JIL_OPTION_H_
#define JIL_OPTION_H_
#pragma once

#include <string>
#include "wx/string.h"
#include "wx/font.h"
#include "editor/defs.h"

namespace jil {

// Option names.
// Consistent with names in config file.

// Names of application options.
const char* const CJK = "cjk";
const char* const FILE_ENCODING = "file_encoding";
const char* const FONT_NAME = "font_name";
const char* const FONT_SIZE = "font_size";
const char* const THEME = "theme";
const char* const SWITCH_CWD = "switch_cwd";
const char* const RESTORE_FILES = "restore_files";
const char* const SHOW_FULL_PATH = "show_full_path";

// Names of editor options.
const char* const WRAP = "wrap";
const char* const SHIFT_WIDTH = "shift_width";
const char* const TAB_STOP = "tab_stop";
const char* const EXPAND_TAB = "expand_tab";
const char* const SHOW_NUMBER = "show_number";
const char* const SHOW_SPACE = "show_space";
const char* const SHOW_HSCROLLBAR = "show_hscrollbar";
const char* const OPERATORS = "operators";
const char* const DELIMITERS = "delimiters";
const char* const RULERS = "rulers";
const char* const INDENT_KEYS = "indent_keys";

// Application options.
class Options {
 public:
  Options()
      : cjk_filters(0)
      , switch_cwd(false)
      , restore_files(true)
      , show_full_path(true) {
  }

  int cjk_filters;

  // Encoding for new created file.
  editor::Encoding file_encoding;

  // Font for the text editor.
  wxFont font;

  // Theme name.
  wxString theme;

  // Switch Current Working Dir when switch text page.
  bool switch_cwd;

  // Remember last open files.
  bool restore_files;

  // Show file full path in the title bar.
  bool show_full_path;
};

}  // namespace jil

#endif  // JIL_OPTION_H_
