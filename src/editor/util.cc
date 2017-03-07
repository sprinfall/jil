#include "editor/util.h"
#include "wx/clipbrd.h"
#include "wx/filename.h"
#include "wx/intl.h"
#include "base/string_util.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

bool SubStringEquals(const std::wstring& str, size_t off, const std::wstring& sub, WcsNCmp cmp) {
  if (off + sub.length() <= str.length()) {
    if (cmp(sub.c_str(), &str[off], sub.length()) == 0) {
      return true;
    }
  }
  return false;
}

Coord CountCharAfter(const std::wstring& str, Coord i, wchar_t c) {
  Coord j = i + 1;
  Coord size = CoordCast(str.size());
  for (; j < size; ++j) {
    if (str[j] != c) {
      break;
    }
  }
  return j - i - 1;
}

bool IsUnescapedBackSlash(const std::wstring& str, size_t i) {
  // Count '\' backward. It's unescaped if the number is odd.
  size_t count = 0;

  while (str[i] == L'\\') {
    ++count;
    if (i == 0) {
      break;
    }
    --i;
  }

  return (count % 2 == 1);
}

bool ParseLuaError(const std::string& what, int* ln, std::string* msg) {
  size_t p1 = what.find_last_of(':');
  if (p1 == std::string::npos) {
    return false;
  }

  *msg = what.substr(p1 + 2);  // +2 for ': '

  if (p1 == 0) {
    return false;
  }
  --p1;

  size_t p2 = what.find_last_of(':', p1);
  if (p2 == std::string::npos) {
    return false;
  }

  std::string ln_str = what.substr(p2 + 1, p1 - p2 - 1);
  *ln = base::LexicalCast<int>(ln_str, 0);

  return (*ln != 0);
}

////////////////////////////////////////////////////////////////////////////////

TimeIt::TimeIt(const wxString& msg) : msg_(msg) {
}

void TimeIt::Start() {
  start_dt_ = wxDateTime::UNow();
  msg_.Clear();
}

void TimeIt::End() {
  wxDateTime end_dt = wxDateTime::UNow();
  if (!start_dt_.IsValid()) {
    return;
  }

  wxTimeSpan ts = end_dt.Subtract(start_dt_);

  msg_ += wxT(" (");
  msg_ += ts.GetMilliseconds().ToString() + wxT("ms)");
}

wxString RectString(const wxRect& rect) {
  return wxString::Format(wxT("%d, %d, %d, %d"), rect.x, rect.y, rect.width, rect.height);
}

////////////////////////////////////////////////////////////////////////////////

static const Encoding kEncodings[ENCODING_COUNT] = {
  { ENCODING_UTF8,            wxFONTENCODING_UTF8,          "utf-8",          "UTF-8"         },
  { ENCODING_UTF8_BOM,        wxFONTENCODING_UTF8,          "utf-8 bom",      "UTF-8 BOM"     },
  { ENCODING_UTF16_BE,        wxFONTENCODING_UTF16BE,       "utf-16be",       "UTF-16 BE"     },
  { ENCODING_UTF16_LE,        wxFONTENCODING_UTF16LE,       "utf-16le",       "UTF-16 LE"     },

  // NOTE: wxWidgets (libiconv) doesn't provide wxFONTENCODING_GB18030,
  // but wxFONTENCODING_GB2312 works.
  { ENCODING_GB18030,         wxFONTENCODING_GB2312,        "gb18030",        "GB18030" },

  { ENCODING_BIG5,            wxFONTENCODING_BIG5,          "big5",           "Big5"          },
  { ENCODING_SHIFT_JIS,       wxFONTENCODING_SHIFT_JIS,     "shift_jis",      "Shift-JIS"     },
  { ENCODING_EUC_JP,          wxFONTENCODING_EUC_JP,        "euc-jp",         "EUC-JP"        },
  { ENCODING_KOI8_R,          wxFONTENCODING_KOI8,          "koi8-r",         "KOI8-R"        },
  { ENCODING_ISO_8859_1,      wxFONTENCODING_ISO8859_1,     "iso-8859-1",     "ISO 8859-1"    },
  { ENCODING_ISO_8859_2,      wxFONTENCODING_ISO8859_2,     "iso-8859-2",     "ISO 8859-2"    },
  { ENCODING_ISO_8859_5,      wxFONTENCODING_ISO8859_5,     "iso-8859-5",     "ISO 8859-5"    },
  { ENCODING_ISO_8859_7,      wxFONTENCODING_ISO8859_7,     "iso-8859-7",     "ISO 8859-7"    },
  { ENCODING_TIS_620,         wxFONTENCODING_ISO8859_11,    "tis-620",        "TIS 620"       },
  { ENCODING_WINDOWS_1250,    wxFONTENCODING_CP1250,        "windows-1250",   "Windows 1250"  },
  { ENCODING_WINDOWS_1251,    wxFONTENCODING_CP1251,        "windows-1251",   "Windows 1251"  },
  { ENCODING_WINDOWS_1253,    wxFONTENCODING_CP1253,        "windows-1253",   "Windows 1253"  },
  { ENCODING_MAC_CYRILLIC,    wxFONTENCODING_MACCYRILLIC,   "x-mac-cyrillic", "Mac Cyrillic" },
};

const Encoding& GetEncodingById(EncodingId id) {
  return kEncodings[id];
}

Encoding GetEncodingByName(const std::string& name) {
  if (name.empty()) {
    return kEncodings[ENCODING_ISO_8859_1];
  }

  for (int i = 0; i < ENCODING_COUNT; ++i) {
    if (kEncodings[i].name == name) {
      return kEncodings[i];
    }
  }

  return Encoding();
}

std::wstring GetEol(FileFormat ff) {
  switch (ff) {
  case FF_CRLF:
    return L"\r\n";
  case FF_LF:
    return L"\n";
  case FF_CR:
    return L"\r";
  default:
    return L"";
  }
}

////////////////////////////////////////////////////////////////////////////////

static const std::string kTextUnitNames[kTextUnitCount] = {
  "", "char", "word", "line", "page", "half_page", "buffer", "selected",
};

static const std::string kSeekTypeNames[kSeekTypeCount] = {
  "", "whole", "begin", "end", "prev", "next",
};

const std::string& UnitName(TextUnit unit) {
  assert(unit < kTextUnitCount);
  return kTextUnitNames[unit];
}

TextUnit UnitFromName(const std::string& name) {
  for (int i = 0; i < kTextUnitCount; ++i) {
    if (kTextUnitNames[i] == name) {
      return static_cast<TextUnit>(i);
    }
  }
  return kNoUnit;
}

const std::string& SeekName(SeekType seek) {
  assert(seek < kSeekTypeCount);
  return kSeekTypeNames[seek];
}

SeekType SeekFromName(const std::string& name) {
  for (int i = 0; i < kSeekTypeCount; ++i) {
    if (kSeekTypeNames[i] == name) {
      return static_cast<SeekType>(i);
    }
  }
  return kNoSeek;
}

////////////////////////////////////////////////////////////////////////////////

bool IsClipboardEmpty() {
  bool is_empty = true;

  if (wxTheClipboard->Open()) {
    if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
      wxTextDataObject data;
      wxTheClipboard->GetData(data);
      is_empty = data.GetText().IsEmpty();
    }
    wxTheClipboard->Close();
  }

  return is_empty;
}

bool MakeDirFully(const wxString& dir) {
  wxFileName dir_fn = wxFileName::DirName(dir);
  if (dir_fn.DirExists()) {
    return true;
  }
  return dir_fn.Mkdir(0777, wxPATH_MKDIR_FULL);
}

}  // namespace editor
}  // namespace jil
