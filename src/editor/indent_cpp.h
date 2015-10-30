#ifndef JIL_EDITOR_INDENT_CPP_H_
#define JIL_EDITOR_INDENT_CPP_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

class TextBuffer;

Coord IndentCpp(const TextBuffer* buffer, Coord ln);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_INDENT_CPP_H_
