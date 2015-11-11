#ifndef JIL_EDITOR_UTIL_H_
#define JIL_EDITOR_UTIL_H_
#pragma once

#include <string>
#include "wx/string.h"
#include "wx/datetime.h"
#include "wx/gdicmn.h"
#include "editor/defs.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

inline bool IsSpace(wchar_t c) {
  return c == kSpaceChar || c == kTabChar;
}

// Returns true if any str.substr(off, n) == sub.
bool SubStringEquals(const std::wstring& str, size_t off, const std::wstring& sub, WcsNCmp cmp = wcsncmp);

// Count char c after index i.
// Example: CountCharAfter(L"abc***d", 2, L'*') returns 3.
Coord CountCharAfter(const std::wstring& str, Coord i, wchar_t c);

// Check if the char at the given index is an unescaped \.
// Examples:
// "\\"[1] -> false
// "a\"[1] -> true
bool IsUnescapedBackSlash(const std::wstring& str, size_t i);

// When luabridge::LuaException is raised from executing the function defined
// in a lua script (e.g., indent.lua), the what() function of the exception
// returns something like this:
//   [string "script content goes here ..."]:256: invalid argument
// This function extracts the line number and the message from it.
bool ParseLuaError(const std::string& what, int* ln, std::string* msg);

////////////////////////////////////////////////////////////////////////////////

template <class C>
void ClearContainer(C* c) {
  // g++ needs typename.
  for (typename C::iterator it = c->begin(); it != c->end(); ++it) {
    delete (*it);
  }
  c->clear();
}

////////////////////////////////////////////////////////////////////////////////

// Measures the time of a code block.
// Usage:
//   TimeIt time_it(wxT("Blah Blah Blah"));
//   time_it.Start();

//   ... (You code here)

//   time_it.End();
//   SomeLogFunc(time_it.result_msg());
class TimeIt {
public:
  TimeIt(const wxString& msg = wxEmptyString);

  void Start();
  void End();

  const wxString& result_msg() const {
    return msg_;
  }

private:
  wxString msg_;
  wxDateTime start_dt_;
};

// Return string "(x, y, w, h)" for a given rect.
// Used by log.
wxString RectString(const wxRect& rect);

////////////////////////////////////////////////////////////////////////////////

const wxString& FileFormatName(FileFormat ff);

Encoding EncodingFromName(const std::string& name);

// Returns EOL string from file format.
std::wstring GetEol(FileFormat file_format);

////////////////////////////////////////////////////////////////////////////////

const std::string& UnitName(TextUnit unit);
TextUnit UnitFromName(const std::string& name);

const std::string& SeekName(SeekType seek);
SeekType SeekFromName(const std::string& name);

////////////////////////////////////////////////////////////////////////////////

bool IsClipboardEmpty();

// Create the dir fully.
// Return true if the dir are finally OK.
bool MakeDirFully(const wxString& dir);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_UTIL_H_
