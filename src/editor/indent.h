#ifndef JIL_EDITOR_INDENT_H_
#define JIL_EDITOR_INDENT_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

class TextBuffer;

namespace cpp {

// Check if the given line is a macro definition.
bool IsLineMacro(const TextBuffer* buffer, Coord ln);

}  // namespace cpp

typedef Coord (*IndentFunc)(const TextBuffer*, Coord);

Coord IndentCfg(const TextBuffer* buffer, Coord ln);
Coord IndentCpp(const TextBuffer* buffer, Coord ln);
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
