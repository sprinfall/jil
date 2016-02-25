#ifndef JIL_PAGE_WINDOW_H_
#define JIL_PAGE_WINDOW_H_
#pragma once

#include "editor/text_window.h"

namespace jil {

class PageState;
class TextPage;

class PageWindow : public editor::TextWindow {
  DECLARE_CLASS(PageWindow)

public:
  explicit PageWindow(TextPage* page);
  virtual ~PageWindow();

  void SetPage(TextPage* page);

  // Add menu items to the edit menu.
  // Different pages might have different edit menu items.
  // E.g., text page has Undo and Redo while find result page doesn't.
  void Page_EditMenu(wxMenu* menu);

  // Get the enable state of the edit menu item.
  bool Page_EditMenuState(int menu_id);

  // Get the enable state of the file menu item and optionaly return the
  // menu item text.
  bool Page_FileMenuState(int menu_id, wxString* text);

  // Handle the menu event.
  bool Page_OnMenu(int menu_id);

  // Special handling of Save As.
  // Save As applies to not only text page, but also tool pages, e.g.,
  // find result page.
  void Page_OnSaveAs();

protected:
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;

  void GetState(PageState* state) const;
  void SetState(const PageState* state);

private:
  TextPage* page_;
};

}  // namespace jil

#endif  // JIL_PAGE_WINDOW_H_
