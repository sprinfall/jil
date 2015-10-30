#ifndef JIL_EDITOR_INDENT_CFG_H_
#define JIL_EDITOR_INDENT_CFG_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

class TextBuffer;

Coord IndentCfg(const TextBuffer* buffer, Coord ln);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_INDENT_CFG_H_
