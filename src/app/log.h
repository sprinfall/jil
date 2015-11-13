#ifndef JIL_LOG_H_
#define JIL_LOG_H_
#pragma once

#include <list>
#include <memory>
#include "wx/string.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

class LogBuffer {
public:
  static LogBuffer* Get();

  bool Add(const wxString& log);

private:
  LogBuffer() {
  }

private:
  std::list<wxString> logs_;
};

////////////////////////////////////////////////////////////////////////////////

class LogUser {
public:
  LogUser();
  virtual ~LogUser();

  void Log(const wxString& log);
};

}  // namespace jil

#endif  // JIL_LOG_H_
