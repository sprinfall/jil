#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

#include "wx/string.h"

namespace jil {

namespace editor {
class TextBuffer;
class TextView;
}  // namespace editor
 
class TextPage {
public:
  enum Flag {
    kModified = 1,
    kUntitled,
  };

public:
  TextPage(editor::TextBuffer* buffer);
  ~TextPage();

  editor::TextBuffer* buffer() const {
    return buffer_;
  }

  editor::TextView* view() const {
    return view_;
  }

  // Activate/deactivate this page.
  void Page_Activate(bool active);

  // Page type ID.
  //wxString Page_Type() const;

  // Page label displayed in tab.
  wxString Page_Label() const;

  // Page description displayed, e.g., in tab tooltip.
  wxString Page_Description() const;

  // See enum Flag.
  int Page_Flags() const;

private:
  editor::TextBuffer* buffer_;
  editor::TextView* view_;
};

}  // namespace jil

#endif  // JIL_TEXT_PAGE_H_
