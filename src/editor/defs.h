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
  FF_CRLF,    // Windows, Dos
  FF_LF,      // Unix, Mac OS X
  FF_CR,      // Mac OS 9
};

// Example: { "cpp", "C++" }
class FileType {
public:
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
#  define FF_DEFAULT FF_CRLF
#else
#  define FF_DEFAULT FF_LF
#endif

const wchar_t LF = L'\n';  // 0x0A
const wchar_t CR = L'\r';  // 0x0D

// Commonly used constant chars.
const wchar_t kTabChar = L'\t';
const wchar_t kSpaceChar = L' ';
const wchar_t kNilChar = L'\0';

enum IndentType {
  kTabIndent,
  kSpaceIndent,
  kMixedIndent,
};

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

enum EncodingId {
  ENCODING_UTF8 = 0,
  ENCODING_UTF8_BOM,
  ENCODING_UTF16_BE,
  ENCODING_UTF16_LE,
  ENCODING_GB18030,
  ENCODING_BIG5,
  ENCODING_SHIFT_JIS,
  ENCODING_EUC_JP,
  ENCODING_KOI8_R,
  ENCODING_ISO_8859_1,
  ENCODING_ISO_8859_2,
  ENCODING_ISO_8859_5,
  ENCODING_ISO_8859_7,
  ENCODING_TIS_620,
  ENCODING_WINDOWS_1250,
  ENCODING_WINDOWS_1251,
  ENCODING_WINDOWS_1253,
  ENCODING_MAC_CYRILLIC,
  ENCODING_COUNT
};

struct Encoding {
  EncodingId id;
  wxFontEncoding value;
  std::string name;         // E.g., "utf-8"
  wxString display_name;    // E.g., "UTF-8"
};

// NOTE: Name is the unique identifier.
inline bool operator==(const Encoding& lhs, const Encoding& rhs) {
  return (lhs.id == rhs.id);
}

inline bool operator!=(const Encoding& lhs, const Encoding& rhs) {
  return (lhs.id != rhs.id);
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
