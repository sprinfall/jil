#ifndef EDITOR_TEXT_LISTENER_H_
#define EDITOR_TEXT_LISTENER_H_

#include "editor/text_range.h"

namespace editor {

enum LineChangeType {
  kLineUpdated    = 1,
  kLineAdded      = 2,

  // NOTE: The line number in the change data will be invalid.
  kLineDeleted    = 4,

  kLineRefresh    = 8,
};

enum ChangeType {
  kEncodingChange       = 1,
  kFileNameChange       = 1 << 1,
  kModifiedStateChange  = 1 << 2,
  kFileFormatChange     = 1 << 3,
  kFileTypeChange       = 1 << 4,
  kTabOptionsChange     = 1 << 5,
  kViewOptionsChange    = 1 << 6,
};

class LineChangeData : public LineRange {
public:
  // Single line.
  explicit LineChangeData(Coord first_line = 0, Coord last_line = 0)
      : LineRange(first_line, last_line) {
  }

  explicit LineChangeData(const LineRange& line_range)
      : LineRange(line_range) {
  }
};

// Implement this interface and attach to a text buffer to listen to
// the changes of it.
class TextListener {
public:
  virtual ~TextListener() {
  }

  virtual void OnBufferLineChange(LineChangeType type,
                                  const LineChangeData& data) = 0;

  virtual void OnBufferChange(ChangeType type) = 0;
};

}  // namespace editor

#endif  // EDITOR_TEXT_LISTENER_H_
