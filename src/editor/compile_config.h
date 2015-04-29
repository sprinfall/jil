#ifndef JIL_EDITOR_COMPILE_CONFIG_H_
#define JIL_EDITOR_COMPILE_CONFIG_H_
#pragma once

#include "wx/defs.h"

// Use C++11 features or not.
#define JIL_USE_CPP11 0

// Underline each lex element (when wrap is off).
// Test only.
#define JIL_TEST_UNDERLINE_LEX_ELEMENT 0

// Rectangle selection. Not fully implemented.
#define JIL_ENABLE_RECT_SELECT 0

// Compound key shortcut (e.g., Ctrl+K,Ctrl+O) support (Windows only).
// Need to disable accelerator (set wxUSE_ACCEL to 0).
#define JIL_ENABLE_LEADER_KEY 0

#if JIL_ENABLE_LEADER_KEY

#if wxUSE_ACCEL
#error "ACCEL should be disabled!"
#endif  // wxUSE_ACCEL

#endif  // JIL_ENABLE_LEADER_KEY

#endif  // JIL_EDITOR_COMPILE_CONFIG_H_
