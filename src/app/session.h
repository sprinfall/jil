#ifndef JIL_SESSION_H_
#define JIL_SESSION_H_
#pragma once

// Session consists of:
// - Find history
// - Last opened files
// - Last cursor positions
// - Top window states
// - etc.

#include <list>
#include "wx/gdicmn.h"
#include "wx/string.h"
#include "app/defs.h"

namespace jil {

class Setting;
class SplitNode;

class Session {
public:
  Session();
  ~Session();

  bool Load(const wxString& file);
  bool Save(const wxString& file);

  //----------------------------------------------------------------------------
  // Book frame states:

  wxRect book_frame_rect() const { return book_frame_rect_; }
  void set_book_frame_rect(const wxRect& rect) { book_frame_rect_ = rect; }

  bool book_frame_maximized() const { return book_frame_maximized_; }
  void set_book_frame_maximized(bool maximized) {
    book_frame_maximized_ = maximized;
  }

  //----------------------------------------------------------------------------
  // Find window states:

  wxRect find_window_rect() const { return find_window_rect_; }
  void set_find_window_rect(const wxRect& rect) { find_window_rect_ = rect; }

  void set_find_history_limit_(size_t find_history_limit) {
    find_history_limit_ = find_history_limit;
  }

  // Add a string to find history.
  // Return true if the find string is new (it didn't exist).
  bool AddFindString(const wxString& find_string) {
    return AddHistoryString(find_string, &recent_find_strings_);
  }
  const std::list<wxString>& recent_find_strings() const {
    return recent_find_strings_;
  }

  // Add a string to replace history.
  // Return true if the replace string is new (it didn't exist).
  bool AddReplaceString(const wxString& replace_string) {
    return AddHistoryString(replace_string, &recent_replace_strings_);
  }
  const std::list<wxString>& recent_replace_strings() const {
    return recent_replace_strings_;
  }

  void set_find_flags(int find_flags) {
    find_flags_ = find_flags;
  }
  int find_flags() const {
    return find_flags_;
  }

  //----------------------------------------------------------------------------
  // Splitter (Layout)

  SplitNode* split_root() const {
    return split_root_;
  }

  void set_split_root(SplitNode* split_root) {
    split_root_ = split_root;
  }

  //----------------------------------------------------------------------------
  // File history

  const std::list<wxString>& opened_files() const {
    return opened_files_;
  }
  void set_opened_files(const std::list<wxString>& opened_files) {
    opened_files_ = opened_files;
  }

  const std::list<wxString>& recent_files() const {
    return recent_files_;
  }
  void set_recent_files(const std::list<wxString>& recent_files) {
    recent_files_ = recent_files;
  }

private:
  bool AddHistoryString(const wxString& s, std::list<wxString>* strings);

  void SaveSplitTree(SplitNode* n, Setting* setting);
  SplitNode* RestoreSplitTree(Setting setting);

private:
  wxRect book_frame_rect_;
  bool book_frame_maximized_;

  wxRect find_window_rect_;

  std::list<wxString> recent_find_strings_;
  std::list<wxString> recent_replace_strings_;

  // Max number of find/replace strings to save.
  size_t find_history_limit_;

  // Last find flags.
  // See enum FindFlag.
  int find_flags_;

  SplitNode* split_root_;

  // The files opened when last time Jil exits.
  // If option "restore_files" is true, these files will be opened on startup.
  std::list<wxString> opened_files_;

  // Recently opened files.
  // Displayed in the Recent Files menu.
  std::list<wxString> recent_files_;
};

}  // namespace jil

#endif  // JIL_SESSION_H_
