#ifndef JIL_UTIL_H_
#define JIL_UTIL_H_
#pragma once

#include <string>
#include "wx/filefn.h"
#include "wx/string.h"
#include "editor/defs.h"

class wxMenu;
class wxWindow;

namespace jil {

// Allocate a new id for text buffer.
size_t NewBufferId();

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
// Others: The same as ExeDir()
wxString ResourceDir();

}  // namespace path

// wxMessageBox wrappers.
void ShowInfo(const wxString& msg, wxWindow* parent = NULL);
void ShowWarning(const wxString& msg, wxWindow* parent = NULL);
void ShowError(const wxString& msg, wxWindow* parent = NULL);

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

// Delete all items of the menu.
void ClearMenuItems(wxMenu* menu);

wxString GetEncodingDisplayName(editor::EncodingId enc_id);

wxString GetFileFormatName(editor::FileFormat ff);

}  // namespace jil

#endif  // JIL_UTIL_H_
