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
const char* const FONT = "font";
const char* const THEME = "theme";
const char* const SWITCH_CWD = "switch_cwd";

// Names of editor options.
const char* const WRAP = "wrap";
const char* const SHIFT_WIDTH = "shift_width";
const char* const TAB_STOP = "tab_stop";
const char* const EXPAND_TAB = "expand_tab";
const char* const SHOW_NUMBER = "show_number";
const char* const SHOW_SPACE = "show_space";
const char* const SHOW_HSCROLLBAR = "show_hscrollbar";
const char* const RULERS = "rulers";

// Application options.
class Options {
 public:
  Options()
      : cjk_filters(0)
      , switch_cwd(false) {
  }

  int cjk_filters;
  editor::Encoding file_encoding;
  wxFont font;
  wxString theme;
  bool switch_cwd;
};

}  // namespace jil

#endif  // JIL_OPTION_H_
