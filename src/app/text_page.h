#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

#include <memory>  // std::shared_ptr

#include "wx/gdicmn.h"
#include "wx/string.h"

#include "editor/option.h"
#include "editor/selection.h"
#include "editor/text_listener.h"
#include "editor/text_point.h"

#include "app/book_page.h"

namespace jil {

namespace editor {

class Action;
class RangeAction;
class TextBuffer;
class WrapHelper;

}  // namespace editor

class PageWindow;

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

  wxPoint view_start;  // First visible position in scroll units.

  editor::Selection selection;

  std::shared_ptr<editor::WrapHelper> wrap_helper;

private:
  void Init() {
    allow_text_change = true;
    caret_point.Set(0, 1);
    max_caret_x = 0;
  }
};

////////////////////////////////////////////////////////////////////////////////

// A non-active text page could be changed by some operations, e.g., Replace All
// in All Pages.
// Implement TextListener so that the wrap info can be updated when changes
// happen.
class TextPage : public editor::TextListener, public BookPage {
public:
  explicit TextPage(editor::TextBuffer* buffer);
  virtual ~TextPage();

  // NOTE: You must call this!
  void set_page_window(PageWindow* page_window);

  //----------------------------------------------------------------------------
  // OVERRIDE of BookPage

  virtual bool Page_HasFocus() const override;
  virtual void Page_SetFocus() override;

  virtual void Page_Activate(bool active) override;
  virtual void Page_Close() override;

  virtual int Page_Type() const override;
  virtual wxString Page_Label() const override;
  virtual wxString Page_Description() const override;
  virtual int Page_Flags() const override;

  virtual void Page_EditMenu(wxMenu* menu) override;
  virtual bool Page_EditMenuState(int menu_id) override;
  virtual bool Page_FileMenuState(int menu_id) override;
  virtual bool Page_OnMenu(int menu_id) override;

  virtual bool Page_Save() override;

  //----------------------------------------------------------------------------
  // Overriddens of editor::TextListener

  virtual void OnBufferLineChange(editor::LineChangeType type, const editor::LineChangeData& data) override;
  virtual void OnBufferChange(editor::ChangeType type) override;

  //----------------------------------------------------------------------------

  editor::TextBuffer* buffer() const {
    return buffer_;
  }

  PageState* state() const {
    return state_;
  }

  // Attach self to the buffer.
  void Attach();

  // Detach self from the buffer.
  void Detach();

  //----------------------------------------------------------------------------

  // Replace seems to be the only operation which will take place on an
  // inactive text page. This function is a wrapper which hides the difference
  // between replace in active page and in inactive pages.
  void Replace(const editor::TextRange& range,
               const std::wstring& replace_str,
               bool grouped);

private:
  // The page window in which this text page will be set to
  // when it becomes active.
  PageWindow* page_window_;

  editor::TextBuffer* buffer_;
  PageState* state_;
};

}  // namespace jil

#endif  // JIL_TEXT_PAGE_H_
