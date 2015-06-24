#ifndef JIL_EDITOR_DEFS_H_
#define JIL_EDITOR_DEFS_H_
#pragma once

#include <string>
#include "wx/defs.h"
#include "wx/fontenc.h"
#include "wx/string.h"
#include "wx/intl.h"

namespace jil {
namespace editor {

const size_t kNpos = static_cast<size_t>(-1);

// Coordinate for text by char and line.
// NOTE: Don't use unsigned type (e.g., size_t) for Coord!
typedef int Coord;

const Coord kInvCoord = -1;

// Explicitly cast Coord, e.g., from size_t.
template <typename T>
inline Coord CoordCast(T value) {
  return static_cast<Coord>(value);
}

template <>
inline Coord CoordCast<size_t>(size_t value) {
  return static_cast<Coord>(value);
}

enum TextDir {
  kForward = 0,
  kBackward
};

enum TextUnit {
  kNoUnit = 0,
  kChar,
  kWord,
  kLine,
  kPage,
  kHalfPage,
  kBuffer,
  kSelected,
  kTextUnitCount,
};

enum SeekType {
  kNoSeek = 0,
  kWhole,
  kBegin,
  kEnd,
  kPrev,
  kNext,
  kSeekTypeCount,
};

// Editor mode.
enum Mode {
  kNormalMode = 1,
  kVisualMode = 2,  // Text selected. "Visual" is borrowed from Vim.

  kAllModes = kNormalMode | kVisualMode,
};

// File format (or EOL, End-Of-Line).
// See http://en.wikipedia.org/wiki/Newline
enum FileFormat {
  FF_NONE = 0,
  FF_WIN,   // Windows, Dos
  FF_UNIX,  // Unix, Mac OS X
  FF_MAC,   // Mac OS 9
};

#define FF_DIAPLAY_NAME_WIN wxT("Windows (CRLF, \\r\\n)")
#define FF_DIAPLAY_NAME_UNIX wxT("Unix (LF, \\n)")
#define FF_DIAPLAY_NAME_MAC wxT("Mac OS 9 (CR, \\r)")

// Example: { "cpp", "C++" }
struct FileType {
  FileType() {
  }

  FileType(const wxString& _id, const wxString& _name)
      : id(_id), name(_name) {
  }

  wxString id;
  wxString name;
};

enum FileError {
  kNoError = 0,
  kIOError,
  kEmptyError,
  kEncodingError
};

#if defined (__WXMSW__)
#  define FF_DEFAULT FF_WIN
#else
#  define FF_DEFAULT FF_UNIX
#endif

const wchar_t LF = L'\n';  // 0x0A
const wchar_t CR = L'\r';  // 0x0D

// Commonly used constant chars.
const wchar_t kTabChar = L'\t';
const wchar_t kSpaceChar = L' ';
const wchar_t kNilChar = L'\0';

// Jil uses Mozilla 'uchardet' to detect file encoding.
// Uchardet supports the following character sets:
// - ISO-8859-2 (European languages)
// - ISO-8859-5 (European languages)
// - ISO-8859-7 (European languages)
// - windows-1250
// - windows-1251
// - windows-1253
// - Big5 (Chinese)
// - EUC-JP (Japanese)
// - (X) EUC-KR (Korean)
// - (X) x-euc-tw (Chinese, EUC-TW)
// - HZ-GB2312 (Chinese) // Actually GB18030
// - (X) ISO-2022-CN (Chinese)
// - (X) ISO-2022-KR (Korean)
// - (X) ISO-2022-JP (Japanese)
// - UTF-8
// - Shift_JIS (Japanese)
// - UTF-16BE
// - UTF-16LE
// - KOI8-R (European languages, Russian)
// - x-mac-cyrillic (European languages)
// - (X) IBM866
// - (X) IBM855
// - TIS-620 (Thai) (ISO/IEC 8859-11)
// - ASCII (ISO-8859-1)

// References:
// ISO-2022: http://en.wikipedia.org/wiki/ISO/IEC_2022
// EUC: http://en.wikipedia.org/wiki/Extended_Unix_Code

// Here's a list of all supported encodings with names in lower case:
const std::string ENCODING_NAME_UTF8 = "utf-8";
const std::string ENCODING_NAME_UTF8_BOM = "utf-8 bom";
const std::string ENCODING_NAME_UTF16_BE = "utf-16be";
const std::string ENCODING_NAME_UTF16_LE = "utf-16le";
const std::string ENCODING_NAME_GB18030 = "gb18030";
const std::string ENCODING_NAME_BIG5 = "big5";
const std::string ENCODING_NAME_SHIFT_JIS = "shift_jis";
const std::string ENCODING_NAME_EUC_JP = "euc-jp";
const std::string ENCODING_NAME_KOI8_R = "koi8-r";
const std::string ENCODING_NAME_ISO_8859_1 = "iso-8859-1";  // Special case
const std::string ENCODING_NAME_ISO_8859_2 = "iso-8859-2";
const std::string ENCODING_NAME_ISO_8859_5 = "iso-8859-5";
const std::string ENCODING_NAME_ISO_8859_7 = "iso-8859-7";
const std::string ENCODING_NAME_TIS_620 = "tis-620";
const std::string ENCODING_NAME_WINDOWS_1250 = "windows-1250";
const std::string ENCODING_NAME_WINDOWS_1251 = "windows-1251";
const std::string ENCODING_NAME_WINDOWS_1253 = "windows-1253";
const std::string ENCODING_NAME_X_MAC_CYRILLIC = "x-mac-cyrillic";

// Encoding display names.
// Define display names with MACRO because most of them have to be translated.
#define ENCODING_DISPLAY_NAME_UTF8 wxT("UTF-8")
#define ENCODING_DISPLAY_NAME_UTF8_BOM wxT("UTF-8 BOM")
#define ENCODING_DISPLAY_NAME_UTF16_BE wxT("UTF-16 BE")
#define ENCODING_DISPLAY_NAME_UTF16_LE wxT("UTF-16 LE")
#define ENCODING_DISPLAY_NAME_GB18030 _("Chinese (GB18030)")
#define ENCODING_DISPLAY_NAME_BIG5 _("Chinese (Big5)")
#define ENCODING_DISPLAY_NAME_SHIFT_JIS _("Japanese (Shift-JIS)")
#define ENCODING_DISPLAY_NAME_EUC_JP _("Japanese (EUC-JP)")
#define ENCODING_DISPLAY_NAME_KOI8_R _("Cyrillic (KOI8-R)")
#define ENCODING_DISPLAY_NAME_ISO_8859_1 _("Western (ISO 8859-1)")
#define ENCODING_DISPLAY_NAME_ISO_8859_2 _("Central European (ISO 8859-2)")
#define ENCODING_DISPLAY_NAME_ISO_8859_5 _("Cyrillic (ISO 8859-5)")
#define ENCODING_DISPLAY_NAME_ISO_8859_7 _("Greek (ISO 8859-7)")
#define ENCODING_DISPLAY_NAME_TIS_620 _("Thai (TIS 620)")
#define ENCODING_DISPLAY_NAME_WINDOWS_1250 _("Central European (Windows 1250)")
#define ENCODING_DISPLAY_NAME_WINDOWS_1251 _("Cyrillic (Windows 1251)")
#define ENCODING_DISPLAY_NAME_WINDOWS_1253 _("Greek (Windows 1253)")
#define ENCODING_DISPLAY_NAME_X_MAC_CYRILLIC _("Cyrillic (Mac)")

struct Encoding {
  wxFontEncoding value;
  std::string name;
  wxString display_name;
};

// NOTE: Name is the unique identifier.
inline bool operator==(const Encoding& lhs, const Encoding& rhs) {
  return (lhs.name == rhs.name);
}

inline bool operator!=(const Encoding& lhs, const Encoding& rhs) {
  return (lhs.name != rhs.name);
}

// BOM bytes.
const char* const UTF_8_BOM_BYTES = "\xEF\xBB\xBF";
const char* const UTF_16BE_BOM_BYTES = "\xFE\xFF";
const char* const UTF_16LE_BOM_BYTES = "\xFF\xFE";

// The window might have several intermediate sizes during the layout process.
// A size like (20, 20) doesn't make sense to trigger a re-wrap or refresh.
// And this constant is used in OnSize() for this optimization.
const int kUnreadyWindowSize = 100;  // px

// Compare C-strings with case ignored.
#ifdef __WXMSW__
#define wcsnicmp _wcsnicmp
#else
#define wcsnicmp wcsncasecmp
#endif

typedef int (*WcsNCmp)(const wchar_t*, const wchar_t*, size_t);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_DEFS_H_
