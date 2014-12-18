#ifndef JIL_EDITOR_TEXT_EXTENT_H_
#define JIL_EDITOR_TEXT_EXTENT_H_
#pragma once

#include <string>
#include "wx/defs.h"
#include "wx/string.h"
#include "editor/text_point.h"

class wxFont;
class wxDC;
class wxMemoryDC;

namespace jil {
namespace editor {

// Text extent used to calculate the text size.
class TextExtent {
public:
  TextExtent();
  ~TextExtent();

  void SetFont(const wxFont& font);

  wxCoord GetWidth(const std::wstring& text);

  void GetExtent(const std::wstring& text,
                 wxCoord* x,
                 wxCoord* y,
                 wxCoord* external_leading = NULL);

  // Binary search to get the char index with the given client x coordinate.
  // The return value might >= line length if vspace is true.
  Coord IndexChar(int tab_stop,
                  const std::wstring& line,
                  int client_x,
                  bool vspace);

private:
  // Binary search to get the char index.
  // The range is STL-style: [begin, end).
  // Called by IndexChar().
  Coord IndexCharRecursively(int tab_stop,
                             const std::wstring& line,
                             Coord begin,
                             Coord end,
                             int client_x,
                             bool vspace);

private:
  wxMemoryDC* dc_;
  int char_width_;
};

size_t TailorLabel(const wxDC& dc, const wxString& label, int max_width);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_EXTENT_H_
