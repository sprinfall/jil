#ifndef JIL_EDITOR_INDENT_H_
#define JIL_EDITOR_INDENT_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

class TextBuffer;

class CppIndent {
public:
  CppIndent(const TextBuffer* buffer);

  void set_indent_namespace(bool indent_namespace) {
    indent_namespace_ = indent_namespace;
  }

  void set_indent_case(bool indent_case) {
    indent_case_ = indent_case;
  }

  // Get the indent of the line.
  Coord Indent(Coord ln);

private:
  const TextBuffer* buffer_;

  // Indent options.
  bool indent_namespace_;
  bool indent_case_;

  // For easy access.
  Coord tab_stop_;
  Coord shift_width_;
};


}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_INDENT_H_
