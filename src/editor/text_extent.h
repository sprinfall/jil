#ifndef JIL_EDITOR_TEXT_EXTENT_H_
#define JIL_EDITOR_TEXT_EXTENT_H_
#pragma once

#include <string>
#include "wx/defs.h"
#include "wx/gdicmn.h"
#include "wx/string.h"
#include "editor/text_point.h"

class wxFont;
class wxDC;
class wxMemoryDC;

namespace jil {
namespace editor {

// Text extent is used to measure the text size.
class TextExtent {
public:
  TextExtent();
  ~TextExtent();

  void SetFont(const wxFont& font);

  wxSize char_size() const {
    return char_size_;
  }

  wxCoord GetWidth(const std::wstring& text);
  wxCoord GetWidth(const std::wstring& text, Coord off, Coord len);

  // Binary search to get the char index with the given client x coordinate.
  // The return value might >= line length if vspace is true.
  Coord IndexChar(int tab_stop,
                  const std::wstring& line,
                  int client_x,
                  bool vspace);

private:
  void UpdateCharSize();

  void GetExtent(const std::wstring& text,
                 Coord off,
                 Coord len,
                 wxCoord* x,
                 wxCoord* y,
                 wxCoord* external_leading = NULL);

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
  wxSize char_size_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_EXTENT_H_
