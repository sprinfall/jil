#ifndef JIL_SESSION_H_
#define JIL_SESSION_H_
#pragma once

// Session consists of:
// - Find history
// - Recent opened files
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

  void set_find_history_limit_(size_t find_history_limit) {
    find_history_limit_ = find_history_limit;
  }

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

 private:
  bool AddHistoryString(const wxString& s, std::list<wxString>* strings);

 public:
  //----------------------------------------------------------------------------
  // Splitter (Layout)

  SplitNode* split_root() const {
    return split_root_;
  }

  void set_split_root(SplitNode* split_root) {
    split_root_ = split_root;
  }

 private:
  void SaveSplitTree(SplitNode* n, Setting* setting);
  SplitNode* RestoreSplitTree(Setting setting);

 public:
  //----------------------------------------------------------------------------
  // Last opened files.

  const std::list<wxString>& last_opened_files() const {
    return last_opened_files_;
  }
  void set_last_opened_files(const std::list<wxString>& last_opened_files) {
    last_opened_files_ = last_opened_files;
  }

  const wxString& last_active_file() const {
    return last_active_file_;
  }
  void set_last_active_file(const wxString& last_active_file) {
    last_active_file_ = last_active_file;
  }

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
  // If option "restore_last_opened_files" is true, these files will be opened
  // on the startup.
  std::list<wxString> last_opened_files_;
  wxString last_active_file_;
};

}  // namespace jil

#endif  // JIL_SESSION_H_
