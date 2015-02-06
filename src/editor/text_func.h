#ifndef JIL_EDITOR_TEXT_FUNC_H_
#define JIL_EDITOR_TEXT_FUNC_H_
#pragma once

#include "editor/defs.h"

namespace jil {
namespace editor {

class TextWindow;

typedef void(*RawTextFunc)(TextWindow*);
typedef void(*RawSeekableTextFunc)(TextWindow*, TextUnit, SeekType);
typedef void(*RawVoidFunc)();

// Text function interface.
class TextFunc {
public:
  virtual ~TextFunc() {
  }

  bool change_text() const {
    return change_text_;
  }
  void set_change_text(bool change_text) {
    change_text_ = change_text;
  }

  virtual void Exec(TextWindow* tw) = 0;

protected:
  TextFunc() : change_text_(false) {
  }

protected:
  // Whether this function changes the text or not.
  bool change_text_;
};

// Use with predefined seekable text functions.
class SeekableTextFuncWrap : public TextFunc {
public:
  SeekableTextFuncWrap(RawSeekableTextFunc func,
                       TextUnit text_unit,
                       SeekType seek_type)
      : func_(func), text_unit_(text_unit), seek_type_(seek_type) {
  }

  virtual ~SeekableTextFuncWrap() {
  }

  virtual void Exec(TextWindow* tw) override {
    if (func_ != NULL) {
      (*func_)(tw, text_unit_, seek_type_);
    }
  }

private:
  RawSeekableTextFunc func_;
  TextUnit text_unit_;
  SeekType seek_type_;
};

// Use with predefined text functions.
class TextFuncWrap : public TextFunc {
  typedef void(*Func)(TextWindow*);

public:
  TextFuncWrap(RawTextFunc func, bool change_text)
      : func_(func) {
    change_text_ = change_text;
  }

  virtual ~TextFuncWrap() {
  }

protected:
  virtual void Exec(TextWindow* tw) override {
    if (func_ != NULL) {
      (*func_)(tw);
    }
  }

private:
  RawTextFunc func_;
};

// Predefined seekable text functions. Use them with SeekableTextFuncWrap,
// e.g., SeekableTextFuncWrap(Delete, kWord, kNext).

void Move(TextWindow* tw, TextUnit text_unit, SeekType seek_type);
void Delete(TextWindow* tw, TextUnit text_unit, SeekType seek_type);
void Scroll(TextWindow* tw, TextUnit text_unit, SeekType seek_type);
void Select(TextWindow* tw, TextUnit text_unit, SeekType seek_type);

// Predefined text functions. Use them with TextFuncWrap,
// e.g., TextFuncWrap(Undo).

void Undo(TextWindow* tw);
void Redo(TextWindow* tw);
void Cut(TextWindow* tw);
void Copy(TextWindow* tw);
void Paste(TextWindow* tw);
void AutoIndent(TextWindow* tw);
void IncreaseIndent(TextWindow* tw);
void DecreaseIndent(TextWindow* tw);
void Format(TextWindow* tw);
void NewLineBreak(TextWindow* tw);
void NewLineBelow(TextWindow* tw);
void NewLineAbove(TextWindow* tw);
void Comment(TextWindow* tw);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_FUNC_H_
