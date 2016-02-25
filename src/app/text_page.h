#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

#include "wx/string.h"

#include "editor/option.h"
#include "editor/selection.h"
#include "editor/text_point.h"

namespace jil {

namespace editor {
class TextBuffer;
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

private:
  void Init() {
    allow_text_change = false;
    caret_point.Set(0, 1);
    max_caret_x = 0;
  }
};

////////////////////////////////////////////////////////////////////////////////

class TextPage {
public:
  enum Flag {
    kModified = 1,
    kUntitled,
  };

public:
  explicit TextPage(editor::TextBuffer* buffer);
  ~TextPage();

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
  int Page_Flags() const;

private:
  editor::TextBuffer* buffer_;
  PageState* state_;
};

}  // namespace jil

#endif  // JIL_TEXT_PAGE_H_
