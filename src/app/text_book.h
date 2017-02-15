#ifndef JIL_TEXT_BOOK_H_
#define JIL_TEXT_BOOK_H_
#pragma once

#include "app/book_ctrl.h"

namespace jil {

namespace editor {
class Binding;
class Style;
class TextBuffer;
}  // namespace editor

class Options;
class PageWindow;
class TextPage;

TextPage* AsTextPage(BookPage* page);

class TextBook : public BookCtrl {
  DECLARE_EVENT_TABLE()

public:
  TextBook();
  virtual ~TextBook();

  void set_options(Options* options) {
    options_ = options;
  }

  void set_page_theme(const editor::SharedTheme& page_theme) {
    page_theme_ = page_theme;
  }

  void set_style(editor::Style* style) {
    style_ = style;
  }

  void set_binding(editor::Binding* binding) {
    binding_ = binding;
  }

  bool Create(wxWindow* parent, wxWindowID id);

  virtual void ReapplyTheme() override;

  // Override to set focus to page window.
  virtual void SetFocus() override;

  PageWindow* page_window() const {
    return page_window_;
  }

  TextPage* placeholder_page() const {
    return placeholder_page_;
  }

  // Add a page for the buffer.
  // Return the new created page.
  TextPage* AddPage(editor::TextBuffer* buffer, bool active);

  TextPage* ActiveTextPage() const;

  std::vector<TextPage*> TextPages() const;
  std::vector<TextPage*> StackTextPages() const;

protected:
  virtual void DoActivateTab(Tab* tab, bool active) override;
  virtual void DoRemoveTab(Tab* tab) override;
  virtual void DoRemoveAll(Tab* tab) override;

  void Init();
  void CreatePageWindow();
  void CreatePlaceholderPage();

  virtual void HandleTabMouseLeftUp(wxMouseEvent& evt) override;
  virtual void HandleTabMouseMiddleUp(wxMouseEvent& evt) override;
  virtual void HandleTabMouseRightDown(wxMouseEvent& evt) override;
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt) override;
  virtual void HandleTabMouseLeftDClick(wxMouseEvent& evt) override;

  std::vector<TextPage*> TextPagesFromTabs(const TabList& tabs) const;

  // Handle text window event to update and refresh tab area.
  // TODO: Rename
  void OnTextWindowEvent(wxCommandEvent& evt);

protected:
  Options* options_;

  // Style for page window.
  editor::Style* style_;

  // Theme for page window.
  editor::SharedTheme page_theme_;

  editor::Binding* binding_;

  // An empty page used as a placeholder when no text page is added.
  // This ensures that text window always has a valid buffer.
  TextPage* placeholder_page_;

  PageWindow* page_window_;
};

}  // namespace jil

#endif  // JIL_TEXT_BOOK_H_
