#ifndef JIL_BOOK_FRAME_H_
#define JIL_BOOK_FRAME_H_
#pragma once

#include <vector>
#include "wx/frame.h"
#include "wx/arrstr.h"
#include "wx/filename.h"
#include "editor/theme.h"
#include "editor/binding.h"
#include "app/defs.h"
#include "app/option.h"
#include "app/id.h"
#include "app/compile_config.h"

namespace jil {

namespace editor {
class TextPoint;
class TextRange;
class TextWindow;
class TextBuffer;
class FtPlugin;
class Style;
class Binding;
class StatusBar;
}

class Session;
class TextPage;
class BookPage;
class BookCtrl;
class TextBook;
class ToolBook;
class FindWindow;
class FindResultPage;
class Splitter;
class SplitNode;

class BookFrame : public wxFrame {
  DECLARE_EVENT_TABLE()

 public:
  enum ColorId {
    BG = 0,
    COLOR_COUNT,
  };

  BookFrame(Options* options, Session* session);
  bool Create(wxWindow* parent, wxWindowID id, const wxString& title);
  virtual ~BookFrame();

  virtual bool Show(bool show = true) override;

  // Must be called before Create().
  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  void set_style(editor::Style* style) {
    style_ = style;
  }

  void set_binding(editor::Binding* binding) {
    binding_ = binding;
  }

  // \param silent Don't popup error message box on failure.
  // Return the text page of this file.
  TextPage* OpenFile(const wxFileName& fn_object, bool active, bool silent);

  // \param silent Don't popup error message box on failure.
  // Return the text page of this file.
  TextPage* OpenFile(const wxString& file_name, bool active, bool silent);

  // \param silent Don't popup error message box on failure.
  void OpenFiles(const wxArrayString& file_names, bool silent);

  // File menu operations.
  void FileNew();
#if JIL_MULTIPLE_WINDOW
  void FileNewWindow();
#endif  // JIL_MULTIPLE_WINDOW

  void FileOpen();

  void FileClose();
  void FileCloseAll();

  void FileSave();
  void FileSaveAs();
  void FileSaveAll();

  size_t PageCount() const;

  void SwitchToNextPage();
  void SwitchToPrevPage();

  void SwitchToNextStackPage();
  void SwitchToPrevStackPage();

  void ShowFind();
  void ShowReplace();

  TextPage* ActiveTextPage() const;
  editor::TextBuffer* ActiveBuffer() const;

  void FindInActivePage(const std::wstring& str, int flags);
#if 0
  void FindInAllPages(const std::wstring& str, int flags);
#endif  // 0

  void FindAllInActivePage(const std::wstring& str, int flags);
#if 0
  void FindAllInAllPages(const std::wstring& str, int flags);
#endif  // 0

  void ReplaceInActivePage(const std::wstring& str,
                           const std::wstring& replace_str,
                           int flags);

  void ReplaceAllInActivePage(const std::wstring& str,
                              const std::wstring& replace_str,
                              int flags);

 private:
  // Return the matched result range.
  editor::TextRange Find(TextPage* text_page,
                         const std::wstring& str,
                         const editor::TextPoint& point,
                         int flags,
                         bool cycle);

  // Find all in the given buffer.
  void FindAll(const std::wstring& str,
               editor::TextBuffer* buffer,
               int flags,
               TextPage* fr_page);

 protected:
  void OnSize(wxSizeEvent& evt);

  // Layout child windows.
  void UpdateLayout();

  void RestoreSplitTree(SplitNode* n);
  SplitNode* CreateDefaultSplitTree();

#if JIL_MULTIPLE_WINDOW
  void OnActivate(wxActivateEvent& evt);
#endif  // JIL_MULTIPLE_WINDOW

  void OnKeyDownHook(wxKeyEvent& evt);
  // Return true if the event is handled.
  bool HandleKeyDownHook(wxKeyEvent& evt);

  void OnMenuFile(wxCommandEvent& evt);
  void OnQuit(wxCommandEvent& evt);

  void OnMenuEdit(wxCommandEvent& evt);
  void OnMenuTools(wxCommandEvent& evt);
  void OnMenuHelp(wxCommandEvent& evt);

  // Return true if the command function is found and executed.
  bool ExecFuncByMenu(int menu);

  // Update the enable state of menu items.
  void OnFileUpdateUI(wxUpdateUIEvent& evt);
  void OnEditUpdateUI(wxUpdateUIEvent& evt);

  void OnClose(wxCloseEvent& evt);

  // BookCtrl tab right-click menu event handler.
  void OnBookRClickMenu(wxCommandEvent& evt);

  // Text page(s) added or removed.
  // TODO: Rename to OnTextBookPageChange
  void OnBookPageChange(wxCommandEvent& evt);
  // Active text page switch.
  // TODO: Rename to OnTextBookPageSwitch
  void OnBookPageSwitch(wxCommandEvent& evt);

  // Tool page(s) added or removed.
  void OnToolBookPageChange(wxCommandEvent& evt);

  // Update status fields according to active text page.
  void UpdateStatusFields();
  // Update title according to active text page.
  void UpdateTitle();

  // Example: "3/120, 27"
  wxString FormatCaretString(TextPage* page) const;

  // Handle events from text window/page.
  void OnTextWindowEvent(wxCommandEvent& evt);

  // Handle events from find result page.
  void OnFindResultPageEvent(wxCommandEvent& evt);

  // Status field event handler.
  void OnStatusFieldClick(wxCommandEvent& evt);
  void PopupEncodingMenu();
  void PopupFileFormatMenu();

  void OnStatusEncodingMenu(wxCommandEvent& evt);
  void OnStatusFileFormatMenu(wxCommandEvent& evt);

  // Return find window if it's shown.
  ::jil::FindWindow* GetFindWindow() const;

  // \param find_window_mode See FindWindow::ViewMode
  void ShowFindWindow(int find_window_mode);

  // Get find result page, create it if necessary.
  FindResultPage* GetFindResultPage();

  // Show tool book and activate the given page.
  void ActivateToolPage(BookPage* page);

 private:
  wxMenuItem* NewMenuItem(wxMenu* menu,
                          int id,
                          const wxString& label,
                          wxItemKind item_kind = wxITEM_NORMAL);
  void LoadMenus();

  bool GetFileMenuEnableState(int menu_id) const;
  bool GetEditMenuEnableState(int menu_id) const;
  bool GetMenuEnableState(int menu_id) const;

  TextBook* ActiveTextBook() const;

  // Create text page with the given buffer.
  TextPage* CreateTextPage(editor::TextBuffer* buffer,
                           wxWindow* parent,
                           wxWindowID id = wxID_ANY);

  TextPage* TextPageByFileName(const wxFileName& fn_object) const;

  void RemovePage(const TextPage* page);
  void RemoveAllPages(const TextPage* except_page = NULL);

  void DoSaveBuffer(editor::TextBuffer* buffer);

 private:
  Options* options_;
  Session* session_;

  // Splitter is to split sub windows.
  Splitter* splitter_;

  std::vector<TextBook*> text_books_;
  ToolBook* tool_book_;

  editor::StatusBar* status_bar_;

  editor::Style* style_;
  editor::SharedTheme theme_;

  editor::Binding* binding_;
  editor::Key leader_key_;
};

}  // namespace jil

#endif  // JIL_BOOK_FRAME_H_
