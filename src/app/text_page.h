#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

#include <memory>  // std::shared_ptr

#include "wx/string.h"

#include "editor/option.h"
#include "editor/selection.h"
#include "editor/text_listener.h"
#include "editor/text_point.h"

namespace jil {

namespace editor {
class TextBuffer;
class WrapHelper;
}  // namespace editor
 
////////////////////////////////////////////////////////////////////////////////

// State (or context) of text page.
class PageState {
public:
  PageState() {
    Init();
  }

  bool allow_text_change;

  editor::ViewOptions view_options;

  editor::TextPoint caret_point;
  editor::Coord max_caret_x;

  editor::Selection selection;

  std::shared_ptr<editor::WrapHelper> wrap_helper;

private:
  void Init() {
    allow_text_change = false;
    caret_point.Set(0, 1);
    max_caret_x = 0;
  }
};

////////////////////////////////////////////////////////////////////////////////

// A non-active text page could be changed by some operations (e.g., Replace All).
// Implement TextListener so that the wrap info can be updated when changes
// happen.
class TextPage : public editor::TextListener {
public:
  enum Flag {
    kModified = 1,
    kUntitled,
  };

public:
  explicit TextPage(editor::TextBuffer* buffer);
  virtual ~TextPage();

  // Overriddens of TextListener:
  virtual void OnBufferLineChange(editor::LineChangeType type, const editor::LineChangeData& data) override;
  virtual void OnBufferChange(editor::ChangeType type) override;

  editor::TextBuffer* buffer() const {
    return buffer_;
  }

  PageState* state() const {
    return state_;
  }

  // Page label displayed in tab.
  wxString GetLabel() const;

  // Page description displayed, e.g., in tab tooltip.
  wxString GetDescription() const;

  // See enum Flag.
  int GetFlags() const;

  // Attach self to buffer.
  void Attach();

  // Detach self from the buffer.
  void Detach();

private:
  editor::TextBuffer* buffer_;
  PageState* state_;
};

}  // namespace jil

#endif  // JIL_TEXT_PAGE_H_
