#include "app/find_thread.h"
#include "app/book_frame.h"

namespace jil {

FindThread::FindThread(BookFrame* handler)
    : wxThread(wxTHREAD_DETACHED)
    , book_frame_(handler) {
}

wxThread::ExitCode FindThread::Entry() {
  if (str_.empty()) {
    return static_cast<wxThread::ExitCode>(0);
  }

  size_t count = files_.GetCount();
  for (size_t i = 0; i < count; ++i) {
    if (TestDestroy()) {
      break;
    }

    int new_fr_lines = book_frame_->AsyncFindInFile(str_, flags_, files_[i]);
    QueueEvent(new_fr_lines, files_[i]);  // Update
  }

  QueueEvent(wxNOT_FOUND, wxEmptyString);  // Finish
  return 0;
}

FindThread::~FindThread() {
  // The thread is being destroyed.
  // Make sure not to leave dangling pointers around.
  wxCriticalSectionLocker locker(book_frame_->find_thread_cs_);
  book_frame_->find_thread_ = NULL;
}

void FindThread::QueueEvent(int new_fr_lines, const wxString& file) {
  wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, ID_FIND_THREAD_EVENT);
  evt->SetString(file);
  evt->SetInt(new_fr_lines);
  wxQueueEvent(book_frame_, evt);
}

}  // namespace jil
