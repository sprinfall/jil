#ifndef JIL_OPTION_H_
#define JIL_OPTION_H_
#pragma once

#include <string>
#include "wx/string.h"
#include "wx/font.h"
#include "editor/defs.h"

namespace jil {

enum FontArea {
  kFont_Text = 0,
  kFont_Tab,
  kFont_Status,
  kFont_Count,
};

// Application options.
class Options {
public:
  Options() {
    Init();
  }

  void Init() {
    cjk_filters = 0;
    switch_cwd = false;
    restore_files = true;
    show_full_path = true;
  }

  int cjk_filters;

  // Encoding for new created file.
  editor::Encoding file_encoding;

  // Fonts for different areas.
  wxFont fonts[kFont_Count];

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
