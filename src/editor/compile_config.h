#ifndef JIL_EDITOR_COMPILE_CONFIG_H_
#define JIL_EDITOR_COMPILE_CONFIG_H_
#pragma once

// Use C++11 features or not.
#define JIL_USE_CPP11 0

// The quote start might have to be matched using regex, which is less
// efficient.
// Currently, no language needs this. So disable it.
#define JIL_LEX_REGEX_QUOTE_START 0

// Underline each lex element (when wrap is off).
// Test only.
#define JIL_TEST_UNDERLINE_LEX_ELEMENT 0

#endif  // JIL_EDITOR_COMPILE_CONFIG_H_
