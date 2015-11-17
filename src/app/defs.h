#ifndef JIL_DEFS_H_
#define JIL_DEFS_H_
#pragma once

#include "wx/defs.h"
#include "wx/string.h"

#define wxLR (wxLEFT|wxRIGHT)
#define wxTB (wxTOP|wxBOTTOM)
#define wxLTR (wxLEFT|wxTOP|wxRIGHT)
#define wxLRB (wxLEFT|wxRIGHT|wxBOTTOM)

#define kMaxFileTypes 256
#define kMaxThemes 32

namespace jil {

enum FindFlag {
  kFind_UseRegex = 1,
  kFind_CaseSensitive = 2,
  kFind_MatchWord = 4,
  kFind_Reversely = 8,  // TODO: Remove
};

enum FindLocation {
  kCurrentPage = 0,
  kAllPages,
  kSelection,
  kLocationCount,
};

// Prefix of internal file type ID.
extern const wxString kInternalFtIdPrefix;

// Internal file type IDs.
extern const wxString kFtId_FindResult;

// Extra data for each line in the find result buffer.
// Saved in TextLine.extra_data_.
class FrExtraData {
public:
  wxString file_path;

  // For new buffers/files which don't have file path.
  size_t buffer_id;

  // Source line id.
  size_t line_id;
};

// Top level theme IDs.
enum ThemeId {
  THEME_BOOK_FRAME = 0,
  THEME_TEXT_BOOK,
  THEME_TEXT_PAGE,
  THEME_FIND_PANEL,
  THEME_STATUS_BAR,
  THEME_NAVIGATION_DIALOG,
  THEME_COUNT,
};

// Split parts.
const char* const kSplit_Text = "text";  // Text book
const char* const kSplit_Tool = "tool";  // Tool book

enum FontType {
  FONT_TEXT = 0,
  FONT_LINE_NR,
  FONT_TABS,
  FONT_STATUS_BAR,
  FONT_COUNT,
};

}  // namespace jil

#endif  // JIL_DEFS_H_
