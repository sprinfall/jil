#include "app/find_thread.h"
#include "app/book_frame.h"

namespace jil {

FindThread::FindThread(BookFrame* book_frame)
    : wxThread(wxTHREAD_DETACHED)
    , book_frame_(book_frame) {
}

wxThread::ExitCode FindThread::Entry() {
  if (str_.empty()) {
    return 0;
  }

  size_t count = files_.GetCount();
  for (size_t i = 0; i < count; ++i) {
    if (TestDestroy()) {
      break;
    }

    // Update status.
    int new_fr_lines = book_frame_->AsyncFindInFile(str_, flags_, files_[i]);
    QueueEvent(new_fr_lines, files_[i]);
  }

  // Finished.
  QueueEvent(wxNOT_FOUND, wxEmptyString);
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
