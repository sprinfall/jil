#ifndef JIL_TEXT_BOOK_H_
#define JIL_TEXT_BOOK_H_
#pragma once

#include "app/book_ctrl.h"

namespace jil {

class TextPage;

class TextBook : public BookCtrl {
  DECLARE_EVENT_TABLE()

 public:
  explicit TextBook(const editor::SharedTheme& theme);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~TextBook();

  virtual bool AddPage(BookPage* page, bool active = false) override;

  TextPage* ActiveTextPage() const;

  std::vector<TextPage*> TextPages() const;
  std::vector<TextPage*> StackTextPages() const;

 protected:
  virtual void HandleTabMouseLeftDown(wxMouseEvent& evt) override;
  virtual void HandleTabMouseLeftUp(wxMouseEvent& evt) override;
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt) override;
  virtual void HandleTabMouseLeftDClick(wxMouseEvent& evt) override;

  void OnRClickMenu(wxCommandEvent& evt);

  // Handle text window event to update and refresh tab area.
  void OnTextWindowEvent(wxCommandEvent& evt);
};

}  // namespace jil

#endif  // JIL_TEXT_BOOK_H_
