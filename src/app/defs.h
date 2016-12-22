#ifndef JIL_DEFS_H_
#define JIL_DEFS_H_
#pragma once

#include "wx/defs.h"
#include "wx/string.h"

#include "editor/defs.h"

#include "app/compile_config.h"

// App name used for paths, config, and other places the user doesn't see.
#if defined (__WXMSW__) || defined (__WXMAC__)
#define kAppName wxT("Jil")
#else
#define kAppName wxT("jil")
#endif

// App name used in window titles, etc.
#define kAppDisplayName wxT("Jil Text")

// App version.
#define kAppVersion wxT("0.1")


#define wxLR (wxLEFT|wxRIGHT)
#define wxTB (wxTOP|wxBOTTOM)
#define wxLTR (wxLEFT|wxTOP|wxRIGHT)
#define wxLRB (wxLEFT|wxRIGHT|wxBOTTOM)
#define wxALIGN_CV  wxALIGN_CENTER_VERTICAL
#define wxALIGN_CH  wxALIGN_CENTER_HORIZONTAL

#define kMaxFileTypes 256
#define kMaxThemes 32

// File format display names (e.g., in the file format menu).
#define FF_DIAPLAY_NAME_CRLF wxT("Windows (CRLF, \\r\\n)")
#define FF_DIAPLAY_NAME_LF wxT("Unix (LF, \\n)")
#define FF_DIAPLAY_NAME_CR wxT("Mac OS 9 (CR, \\r)")

#define kFindHistoryLimit 10

namespace jil {

enum FindFlag {
  kFind_UseRegex = 1,
  kFind_CaseSensitive = 2,
  kFind_MatchWord = 4,
  kFind_Reversely = 8,
};

enum FindLocation {
  kCurrentPage = 0,
  kAllPages,
  kFolders,
  kFindLocations,
};

// Prefix of all internal file type IDs.
extern const wxString kInternalFtIdPrefix;

// Internal file type IDs.
extern const wxString kFindResultFtId;

// Plain Text file type ID.
// Please use kTrPlainText as file type name.
extern const wxString kTxtFtId;

// Plain Text file type.
extern const editor::FileType kTxtFt;

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
  THEME_POPUP,  // Theme for all popup dialogs, custom menus, etc.
  THEME_COUNT,
};

struct WithPopupTheme {
  enum ColorId {
    COLOR_BG = 0,
    COLOR_FG,
    COLOR_BORDER,
    COLOR_SELECT_BG,
    COLOR_SELECT_BORDER,
    COLOR_SELECT_FG,
    COLORS,
  };
};

// Split parts.
const char* const kSplit_Text = "text";  // Text book
const char* const kSplit_Tool = "tool";  // Tool book

// See BookPage::Page_Type().
enum PageType {
  TEXT_PAGE_TYPE = 0,
  FIND_RESULT_PAGE_TYPE,
};

enum FontType {
  FONT_TEXT = 0,
  FONT_TABS,
  FONT_STATUS_BAR,
  FONT_FIND_PANEL,
  FONT_COUNT,
};

enum BufferId {
  // Buffers opened temporarily for, e.g., searching purpose.
  // See BookFrame::AsyncFindInFile().
  kTempBufferId = 0,

  // Used by the placeholder buffer.
  // See TextBook.placeholder_page_.
  kPlaceholderBufferId,

  kOtherBufferId,
};

}  // namespace jil

#endif  // JIL_DEFS_H_
