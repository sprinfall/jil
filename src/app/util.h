#ifndef JIL_UTIL_H_
#define JIL_UTIL_H_
#pragma once

#include <string>
#include "wx/filefn.h"

class wxWindow;

namespace jil {

class WorkingDirSwitcher {
 public:
  explicit WorkingDirSwitcher(const wxString& new_wd) {
    old_wd_ = wxGetCwd();
    wxSetWorkingDirectory(new_wd);
  }

  ~WorkingDirSwitcher() {
    wxSetWorkingDirectory(old_wd_);
  }

 private:
  wxString old_wd_;
};

namespace path {

// Executable file dir.
// Windows: E.g., C:\Program Files\Jil\
// Mac: <bundle>/Contents/MacOS/
wxString ExeDir();

// Resource dir.
// Mac: <bundle>/Contents/Resources/
// Others: The same as GetExeDir()
wxString ResourceDir();

}  // namespace path

void ErrorMsg(const wxString& msg, wxWindow* parent = NULL);

// Open file containing folder.
void ExploreFile(const wxString& file_path);

// Return the enable state of the bit in the flags.
inline bool GetBit(int flags, int bit) {
  return (flags & bit) != 0;
}

// Enable a bit of the flags and return the new one.
inline int SetBit(int flags, int bit, bool state) {
  if (state) {
    flags |= bit;
  } else {
    flags &= ~bit;
  }
  return flags;
}

}  // namespace jil

#endif  // JIL_UTIL_H_
