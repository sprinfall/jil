#ifndef JIL_DEFS_H_
#define JIL_DEFS_H_
#pragma once

#include "wx/defs.h"

namespace jil {

enum FindFlag {
  kFindUseRegex = 1,
  kFindCaseSensitive = 2,
  kFindMatchWholeWord = 4,
  kFindReversely = 8,
};

// Internal file type IDs.
// Format: "jil-xxx_yyy_zzz"
const char* const kFtId_FindResult = "jil-find_result";

// Top level theme IDs.
// TODO: THEME_TOOL_BOOK
enum ThemeId {
  THEME_BOOK_FRAME = 0,
  THEME_TEXT_BOOK,
  THEME_TEXT_PAGE,
  THEME_STATUS_BAR,
  THEME_NAVIGATION_DIALOG,
  THEME_COUNT,
};

// Split parts.
const char* const kSplit_Text = "text";  // Text books
const char* const kSplit_Tool = "tool";  // Tool book

}  // namespace jil

#endif  // JIL_DEFS_H_
