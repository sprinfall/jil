#ifndef JIL_TEXT_PAGE_H_
#define JIL_TEXT_PAGE_H_
#pragma once

// TextWindow as book page.

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
  virtual wxString Page_Label() const override;
  virtual wxString Page_Description() const override;
  virtual int Page_Flags() const override;

  bool SaveBuffer();

protected:
  // OVERRIDE of editor::TextWindow:
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;

  // Add menu items to the right click menu.
  virtual void FillRClickMenu(wxMenu& menu);

private:
  void PostEvent(wxEventType evt_type);
};

////////////////////////////////////////////////////////////////////////////////

// Convert book page to text page or NULL.
TextPage* AsTextPage(BookPage* page);

// Get all text pages from a book.
std::vector<TextPage*> BookTextPages(BookCtrl* book);

}  // namespace jil

#endif  // JIL_TEXT_PAGE_H_
