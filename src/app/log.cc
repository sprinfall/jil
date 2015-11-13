#include "app/log.h"

namespace jil {

// static
LogBuffer* LogBuffer::Get() {
  static std::auto_ptr<LogBuffer> buffer(new LogBuffer);
  return buffer.get();
}

bool LogBuffer::Add(const wxString& log) {
  logs_.push_back(log);
  return true;
}

////////////////////////////////////////////////////////////////////////////////

LogUser::LogUser() {
}

LogUser::~LogUser() {
}

void LogUser::Log(const wxString& log) {
  LogBuffer::Get()->Add(log);
}

}  // namespace jil
