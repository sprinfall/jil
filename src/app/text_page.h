#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

// Text window as book page.

#include <vector>
#include "editor/text_window.h"
#include "app/book_ctrl.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////

class TextPage : public editor::TextWindow, public BookPage {
  DECLARE_CLASS(TextPage)

public:
  explicit TextPage(editor::TextBuffer* buffer);
  virtual ~TextPage();

  // OVERRIDE of BookPage:
  virtual wxWindow* Page_Window() override { return this; }
  virtual void Page_Activate(bool active) override;
  virtual bool Page_Close() override;
  virtual wxString Page_Type() const override;
  virtual wxString Page_Label() const override;
  virtual wxString Page_Description() const override;
  virtual int Page_Flags() const override;
  virtual void Page_EditMenu(wxMenu* menu) override;

  bool SaveBuffer();

protected:
  // OVERRIDE of editor::TextWindow:
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;
};

////////////////////////////////////////////////////////////////////////////////

// Convert book page to text page or NULL.
TextPage* AsTextPage(BookPage* page);

}  // namespace jil

#endif  // JIL_TEXT_PAGE_H_
