#ifndef JIL_OPTION_CONFIG_H_
#define JIL_OPTION_CONFIG_H_
#pragma once

// Parse option settings.

class wxString;

namespace jil {

class Setting;

namespace editor {
class Options;
}  // namespace editor

// Option names.
// Consistent with names in config file.

// Names of application options.
const char* const CJK = "cjk";
const char* const FILE_ENCODING = "file_encoding";
const char* const THEME = "theme";
const char* const FONTS = "fonts";
const char* const LINE_PADDING = "line_padding";
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
const char* const DELIMITERS = "delimiters";
const char* const RULERS = "rulers";
const char* const INDENT_KEYS = "indent_keys";

class Options;

void ParseAppOptions(const Setting& setting, Options* options);

void ParseEditorOptions(const Setting& setting, editor::Options* options);

}  // namespace jil

#endif  // JIL_OPTION_CONFIG_H_
