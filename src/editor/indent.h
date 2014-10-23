#ifndef JIL_EDITOR_INDENT_H_
#define JIL_EDITOR_INDENT_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

class TextBuffer;

class IndentC {
};

class IndentGo {
};

class IndentPython {
};

class IndentJava {
};

class IndentCSharp {
};

class IndentVB {
};

class IndentXml {
};

class IndentBase {
public:
  IndentBase(const TextBuffer* buffer);
  virtual ~IndentBase() = 0;

  virtual Coord Indent(Coord ln) = 0;

protected:
  const TextBuffer* buffer_;

  Coord tab_stop_;
  Coord shift_width_;
};

class IndentCpp : public IndentBase {
public:
  IndentCpp(const TextBuffer* buffer);

  void set_indent_namespace(bool indent_namespace) {
    indent_namespace_ = indent_namespace;
  }

  void set_indent_case(bool indent_case) {
    indent_case_ = indent_case;
  }

  // Get the indent of the line.
  virtual Coord Indent(Coord ln) override;

private:
  // Indent options.
  bool indent_namespace_;
  bool indent_case_;
};


}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_INDENT_H_
