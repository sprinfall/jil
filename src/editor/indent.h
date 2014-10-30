#ifndef JIL_EDITOR_INDENT_H_
#define JIL_EDITOR_INDENT_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

class TextBuffer;

// Base class for all indent functions.
class IndentFunc {
public:
  virtual ~IndentFunc() {
  }

  void SetBuffer(const TextBuffer* buffer);

  // Get the indent of the line.
  virtual Coord Indent(Coord ln) = 0;

protected:
  const TextBuffer* buffer_;

  Coord tab_stop_;
  Coord shift_width_;
};

////////////////////////////////////////////////////////////////////////////////

class IndentTxt: public IndentFunc {
public:
  IndentTxt();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentCpp : public IndentFunc {
public:
  IndentCpp();

  void set_indent_namespace(bool indent_namespace) {
    indent_namespace_ = indent_namespace;
  }

  void set_indent_case(bool indent_case) {
    indent_case_ = indent_case;
  }

  virtual Coord Indent(Coord ln) override;

private:
  // Indent options.
  bool indent_namespace_;
  bool indent_case_;
};

////////////////////////////////////////////////////////////////////////////////

class IndentJava : public IndentFunc {
public:
  IndentJava();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentCSharp : public IndentFunc {
public:
  IndentCSharp();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentPython : public IndentFunc {
public:
  IndentPython();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentRuby : public IndentFunc {
public:
  IndentRuby();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentGo : public IndentFunc {
public:
  IndentGo();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentJavaScript : public IndentFunc {
public:
  IndentJavaScript();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentXml : public IndentFunc {
public:
  IndentXml();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentHtml : public IndentFunc {
public:
  IndentHtml();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentCss : public IndentFunc {
public:
  IndentCss();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentCue : public IndentFunc {
public:
  IndentCue();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentCfg : public IndentFunc {
public:
  IndentCfg();

  virtual Coord Indent(Coord ln) override;
};

////////////////////////////////////////////////////////////////////////////////

class IndentVB : public IndentFunc {
public:
  IndentVB();

  virtual Coord Indent(Coord ln) override;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_INDENT_H_
