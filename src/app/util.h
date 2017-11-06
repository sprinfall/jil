#ifndef JIL_UTIL_H_
#define JIL_UTIL_H_
#pragma once

#include <string>

#include "wx/filefn.h"
#include "wx/string.h"

#include "ui/button_style.h"

#include "editor/defs.h"
#include "editor/theme.h"

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

// \param lang See wxLanguage
bool IsTraditionalChinese(int lang);

// Create a button style from button theme.
ui::SharedButtonStyle ButtonStyleFromTheme(editor::SharedTheme button_theme);

#if defined(__WXMSW__)

// Get an icon by ID from resources.
// \param icon_id IDI_EDITOR_16
wxIcon MSWGetEmbeddedIcon(int icon_id);

#endif  // __WXMSW__

}  // namespace jil

#endif  // JIL_UTIL_H_
