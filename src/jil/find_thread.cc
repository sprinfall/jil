#include "jil/find_thread.h"
#include "jil/book_frame.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

FindThreadBase::FindThreadBase(BookFrame* book_frame,
                               const std::wstring& str,
                               int flags)
    : wxThread(wxTHREAD_DETACHED)
    , book_frame_(book_frame)
    , str_(str)
    , flags_(flags)
    , event_id_(0) {
}

FindThreadBase::~FindThreadBase() {
  // The thread is being destroyed.
  // Make sure not to leave dangling pointers around.
  wxCriticalSectionLocker locker(book_frame_->find_thread_cs_);
  book_frame_->find_thread_ = NULL;
}

bool FindThreadBase::IsReplace() const {
  return event_id_ == ID_REPLACE_THREAD_EVENT;
}

wxThread::ExitCode FindThreadBase::Entry() {
  if (str_.empty()) {
    return 0;
  }

  size_t count = files_.GetCount();
  for (size_t i = 0; i < count; ++i) {
    if (TestDestroy()) {
      break;
    }

    int ivalue = HandleFile(files_[i]);

    QueueEvent(ivalue, files_[i]);
  }

  QueueEvent(wxNOT_FOUND, wxEmptyString);
  return 0;
}

void FindThreadBase::QueueEvent(int ivalue, const wxString& file) {
  wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, event_id_);
  evt->SetString(file);
  evt->SetInt(ivalue);
  wxQueueEvent(book_frame_, evt);
}

////////////////////////////////////////////////////////////////////////////////

FindThread::FindThread(BookFrame* book_frame,
                       const std::wstring& str,
                       int flags)
    : FindThreadBase(book_frame, str, flags) {
  event_id_ = ID_FIND_THREAD_EVENT;
}

FindThread::~FindThread() {
}

int FindThread::HandleFile(const wxString& file) {
  return book_frame_->AsyncFindInFile(str_, flags_, file);
}

////////////////////////////////////////////////////////////////////////////////

ReplaceThread::ReplaceThread(BookFrame* book_frame,
                             const std::wstring& str,
                             const std::wstring& replace_str,
                             int flags)
    : FindThreadBase(book_frame, str, flags)
    , replace_str_(replace_str) {
  event_id_ = ID_REPLACE_THREAD_EVENT;
}

ReplaceThread::~ReplaceThread() {
}

int ReplaceThread::HandleFile(const wxString& file) {
  return book_frame_->AsyncReplaceInFile(str_, replace_str_, flags_, file);
}

}  // namespace jil
