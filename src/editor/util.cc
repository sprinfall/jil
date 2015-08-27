#include "editor/util.h"
#include "wx/clipbrd.h"
#include "wx/filename.h"
#include "wx/intl.h"

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

////////////////////////////////////////////////////////////////////////////////

TimeIt::TimeIt(const wxString& msg) : msg_(msg) {
}

void TimeIt::Start() {
  start_dt_ = wxDateTime::UNow();
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

const wxString& FileFormatName(FileFormat ff) {
  static const wxString kFileFormatNames[4] = {
    wxEmptyString,
    wxT("Win"),
    wxT("Unix"),
    wxT("Mac"),
  };
  return kFileFormatNames[ff];
}

Encoding EncodingFromName(const std::string& name) {
  Encoding encoding;

  if (name.empty() || name == ENCODING_NAME_ISO_8859_1) {
    encoding.value = wxFONTENCODING_ISO8859_1;
    encoding.display_name = ENCODING_DISPLAY_NAME_ISO_8859_1;
  } else if (name == ENCODING_NAME_UTF8) {
    encoding.value = wxFONTENCODING_UTF8;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF8;
  } else if (name == ENCODING_NAME_UTF8_BOM) {
    encoding.value = wxFONTENCODING_UTF8;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF8_BOM;
  } else if (name == ENCODING_NAME_UTF16_BE) {
    encoding.value = wxFONTENCODING_UTF16BE;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF16_BE;
  } else if (name == ENCODING_NAME_UTF16_LE) {
    encoding.value = wxFONTENCODING_UTF16LE;
    encoding.display_name = ENCODING_DISPLAY_NAME_UTF16_LE;
  } else if (name == ENCODING_NAME_GB18030) {
    // NOTE: wxWidgets (libiconv) doesn't provide wxFONTENCODING_GB18030,
    // but wxFONTENCODING_GB2312 works.
    encoding.value = wxFONTENCODING_GB2312;
    encoding.display_name = ENCODING_DISPLAY_NAME_GB18030;
  } else if (name == ENCODING_NAME_BIG5) {
    encoding.value = wxFONTENCODING_BIG5;
    encoding.display_name = ENCODING_DISPLAY_NAME_BIG5;
  } else if (name == ENCODING_NAME_SHIFT_JIS) {
    encoding.value = wxFONTENCODING_SHIFT_JIS;
    encoding.display_name = ENCODING_DISPLAY_NAME_SHIFT_JIS;
  } else if (name == ENCODING_NAME_EUC_JP) {
    encoding.value = wxFONTENCODING_EUC_JP;
    encoding.display_name = ENCODING_DISPLAY_NAME_EUC_JP;
  } else if (name == ENCODING_NAME_KOI8_R) {
    encoding.value = wxFONTENCODING_KOI8;
    encoding.display_name = ENCODING_DISPLAY_NAME_KOI8_R;
  } else if (name == ENCODING_NAME_ISO_8859_2) {
    encoding.value = wxFONTENCODING_ISO8859_2;
    encoding.display_name = ENCODING_DISPLAY_NAME_ISO_8859_2;
  } else if (name == ENCODING_NAME_ISO_8859_5) {
    encoding.value = wxFONTENCODING_ISO8859_5;
    encoding.display_name = ENCODING_DISPLAY_NAME_ISO_8859_5;
  } else if (name == ENCODING_NAME_ISO_8859_7) {
    encoding.value = wxFONTENCODING_ISO8859_7;
    encoding.display_name = ENCODING_DISPLAY_NAME_ISO_8859_7;
  } else if (name == ENCODING_NAME_TIS_620) {
    encoding.value = wxFONTENCODING_ISO8859_11;
    encoding.display_name = ENCODING_DISPLAY_NAME_TIS_620;
  } else if (name == ENCODING_NAME_WINDOWS_1250) {
    encoding.value = wxFONTENCODING_CP1250;
    encoding.display_name = ENCODING_DISPLAY_NAME_WINDOWS_1250;
  } else if (name == ENCODING_NAME_WINDOWS_1251) {
    encoding.value = wxFONTENCODING_CP1251;
    encoding.display_name = ENCODING_DISPLAY_NAME_WINDOWS_1251;
  } else if (name == ENCODING_NAME_WINDOWS_1253) {
    encoding.value = wxFONTENCODING_CP1253;
    encoding.display_name = ENCODING_DISPLAY_NAME_WINDOWS_1253;
  } else if (name == ENCODING_NAME_X_MAC_CYRILLIC) {
    encoding.value = wxFONTENCODING_MACCYRILLIC;
    encoding.display_name = ENCODING_DISPLAY_NAME_X_MAC_CYRILLIC;
  }

  encoding.name = name;
  return encoding;
}

std::wstring GetEol(FileFormat file_format) {
  switch (file_format) {
    case FF_WIN:
      return L"\r\n";
    case FF_UNIX:
      return L"\n";
    case FF_MAC:
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
