#ifndef JIL_PAGE_WINDOW_H_
#define JIL_PAGE_WINDOW_H_
#pragma once

#include "editor/text_window.h"

namespace jil {

class PageState;
class TextPage;

class PageWindow : public editor::TextWindow {
  DECLARE_CLASS(PageWindow)

  friend class TextPage;

public:
  explicit PageWindow(TextPage* page);
  virtual ~PageWindow();

  TextPage* page() const {
    return page_;
  }

  // Switch to the given text page.
  void SetPage(TextPage* page);

  bool IsPagePlaceholder() const;

  void GetEditMenu(wxMenu* menu);
  bool GetEditMenuState(int menu_id);
  bool GetFileMenuState(int menu_id);
  bool HandleMenu(int menu_id);

protected:
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;

  void GetState(PageState* state) const;
  void SetState(PageState* state);

private:
  void OnPageModifiedStateChange(TextPage* page);

private:
  TextPage* page_;  // Always != NULL.
};

}  // namespace jil

#endif  // JIL_PAGE_WINDOW_H_
