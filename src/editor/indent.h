#ifndef JIL_EDITOR_INDENT_H_
#define JIL_EDITOR_INDENT_H_
#pragma once

#include "editor/defs.h"
#include "editor/text_line.h"

namespace jil {
namespace editor {

class TextBuffer;

typedef Coord (*IndentFunc)(const TextBuffer*, Coord);

inline bool StartWith(const TextLine* line,
                      const std::wstring& str,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return line->StartWith(str, ignore_spaces, off);
}

inline bool StartWith(const TextLine* line,
                      const std::wstring& str1,
                      const std::wstring& str2,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return StartWith(line, str1, ignore_spaces, off) ||
         StartWith(line, str2, ignore_spaces, off);
}

inline bool StartWith(const TextLine* line,
                      const std::wstring& str1,
                      const std::wstring& str2,
                      const std::wstring& str3,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return StartWith(line, str1, str2, ignore_spaces, off) ||
         StartWith(line, str3, ignore_spaces, off);
}

inline bool StartWith(const TextLine* line,
                      const std::wstring& str1,
                      const std::wstring& str2,
                      const std::wstring& str3,
                      const std::wstring& str4,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return StartWith(line, str1, str2, str3, ignore_spaces, off) ||
         StartWith(line, str4, ignore_spaces, off);
}

inline bool EndWith(const TextLine* line,
                    const std::wstring& str,
                    bool ignore_comments,
                    bool ignore_spaces,
                    Coord* off = NULL) {
  return line->EndWith(str, ignore_comments, ignore_spaces, off);
}

inline bool EndWith(const TextLine* line,
                    const std::wstring& str1,
                    const std::wstring& str2,
                    bool ignore_comments,
                    bool ignore_spaces,
                    Coord* off = NULL) {
  return EndWith(line, str1, ignore_comments, ignore_spaces, off) ||
         EndWith(line, str2, ignore_comments, ignore_spaces, off);
}

inline bool EndWith(const TextLine* line,
                    const std::wstring& str1,
                    const std::wstring& str2,
                    const std::wstring& str3,
                    bool ignore_comments,
                    bool ignore_spaces,
                    Coord* off = NULL) {
  return EndWith(line, str1, str2, ignore_comments, ignore_spaces, off) ||
         EndWith(line, str3, ignore_comments, ignore_spaces, off);
}

bool Match(const TextLine* line,
           const std::wstring& str,
           bool ignore_comments,
           bool ignore_spaces,
           Coord* off = NULL);

// TODO
Coord IndentCSharp(const TextBuffer* buffer, Coord ln);
Coord IndentCss(const TextBuffer* buffer, Coord ln);
Coord IndentCue(const TextBuffer* buffer, Coord ln);
Coord IndentGo(const TextBuffer* buffer, Coord ln);
Coord IndentHtml(const TextBuffer* buffer, Coord ln);
Coord IndentJava(const TextBuffer* buffer, Coord ln);
Coord IndentJavaScript(const TextBuffer* buffer, Coord ln);
Coord IndentPython(const TextBuffer* buffer, Coord ln);
Coord IndentRuby(const TextBuffer* buffer, Coord ln);
Coord IndentTxt(const TextBuffer* buffer, Coord ln);
Coord IndentVB(const TextBuffer* buffer, Coord ln);
Coord IndentXml(const TextBuffer* buffer, Coord ln);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_INDENT_H_
