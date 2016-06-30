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

  bool IsPagePlaceholder() const;

  void GetEditMenu(wxMenu* menu);
  bool GetEditMenuState(int menu_id);
  bool GetFileMenuState(int menu_id, wxString* text);
  bool HandleMenu(int menu_id);

protected:
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;

  void GetState(PageState* state) const;
  void SetState(PageState* state);

private:
  TextPage* page_;
};

}  // namespace jil

#endif  // JIL_PAGE_WINDOW_H_
