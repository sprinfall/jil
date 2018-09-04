#ifndef EDITOR_COMPILE_CONFIG_H_
#define EDITOR_COMPILE_CONFIG_H_

#include "wx/defs.h"

// Use relite::Regex instead of C++11 regex to scan lex for performance.
// Scan a 2500 lines C++ source file with Release build,
//   - C++/Boost regex: 175ms
//   - relite::Regex: 16ms
#define JIL_LEX_USE_RELITE 1

// Rectangle selection. Not fully implemented.
#define JIL_ENABLE_RECT_SELECT 0

// Use wxCaret or not for TextWindow.
// wxCaret has no native implementation on Mac or GTK, and it's buggy.
// Alternatively, we implement a caret by ourselves.
#define JIL_USE_WX_CARET 0

// Compound key shortcut (e.g., Ctrl+K,Ctrl+O) support (Windows only).
// Need to disable accelerator (set wxUSE_ACCEL to 0).
#define JIL_ENABLE_LEADER_KEY 0

#if JIL_ENABLE_LEADER_KEY

#if wxUSE_ACCEL
#error "ACCEL should be disabled!"
#endif  // wxUSE_ACCEL

#endif  // JIL_ENABLE_LEADER_KEY

#endif  // EDITOR_COMPILE_CONFIG_H_
