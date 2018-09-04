#ifndef JIL_FIND_THREAD_H_
#define JIL_FIND_THREAD_H_

#include "wx/arrstr.h"
#include "wx/thread.h"

namespace jil {

class BookFrame;

////////////////////////////////////////////////////////////////////////////////

class FindThreadBase : public wxThread {
public:
  FindThreadBase(BookFrame* book_frame,
                 const std::wstring& str,
                 int flags);

  virtual ~FindThreadBase();

  // TODO: Avoid copy.
  void set_files(const wxArrayString& files) {
    files_ = files;
  }

  bool IsReplace() const;

protected:
  virtual ExitCode Entry() override;

  virtual int HandleFile(const wxString& file) = 0;

  void QueueEvent(int ivalue, const wxString& file);

protected:
  BookFrame* book_frame_;

  std::wstring str_;  // The string to find.
  int flags_;  // Find flags.
  wxArrayString files_;  // The files in which to find.

  int event_id_;
};

////////////////////////////////////////////////////////////////////////////////

class FindThread : public FindThreadBase {
public:
  FindThread(BookFrame* book_frame,
             const std::wstring& str,
             int flags);

  virtual ~FindThread();

protected:
  virtual int HandleFile(const wxString& file) override;
};

////////////////////////////////////////////////////////////////////////////////

class ReplaceThread : public FindThreadBase {
public:
  ReplaceThread(BookFrame* book_frame,
                const std::wstring& str,
                const std::wstring& replace_str,
                int flags);

  virtual ~ReplaceThread();

protected:
  virtual int HandleFile(const wxString& file) override;

private:
  std::wstring replace_str_;
};

}  // namespace jil

#endif  // JIL_FIND_THREAD_H_
