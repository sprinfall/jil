#ifndef JIL_FIND_THREAD_H_
#define JIL_FIND_THREAD_H_
#pragma once

#include "wx/arrstr.h"
#include "wx/thread.h"

namespace jil {

class BookFrame;

// Thread for Find All.
class FindThread : public wxThread {
public:
  explicit FindThread(BookFrame* book_frame);
  virtual ~FindThread();

  void set_str(const std::wstring& str) {
    str_ = str;
  }

  void set_flags(int flags) {
    flags_ = flags;
  }

  // TODO: Avoid copy
  void set_files(const wxArrayString& files) {
    files_ = files;
  }

protected:
  virtual ExitCode Entry() override;

  void QueueEvent(int new_fr_lines, const wxString& file);

private:
  BookFrame* book_frame_;

  std::wstring str_;  // The string to find.
  int flags_;  // Find flags.
  wxArrayString files_;  // The files in which to find.
};

}  // namespace jil

#endif  // JIL_FIND_THREAD_H_
