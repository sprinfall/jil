#ifndef JIL_EDITOR_BUFFER_LISTENER_H_
#define JIL_EDITOR_BUFFER_LISTENER_H_
#pragma once

#include "editor/text_range.h"

namespace jil {
namespace editor {

enum LineChangeType {
  kLineUpdated    = 1,
  kLineAdded      = 2,

  // NOTE: The line number in the change data will be invalid.
  kLineDeleted    = 4,

  kLineRefresh    = 8,
};

enum ChangeType {
  kEncodingChange = 1,
  kFileNameChange = 2,
  kModifiedChange = 4,  // Modified state change.
  kFileFormatChange = 8,
  kFileTypeChange = 16,
};

class LineChangeData : public LineRange {
public:
  // Single line.
  LineChangeData(Coord first_line = 0, Coord last_line = 0)
      : LineRange(first_line, last_line) {
  }

  explicit LineChangeData(const LineRange& line_range)
      : LineRange(line_range) {
  }
};

// Implement this interface and attach to a text buffer to listen to
// the changes of it.
class BufferListener {
public:
  virtual ~BufferListener() {
  }

  virtual void OnBufferLineChange(LineChangeType type, const LineChangeData& data) = 0;

  virtual void OnBufferChange(ChangeType type) = 0;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_BUFFER_LISTENER_H_
