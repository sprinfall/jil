#ifndef JIL_BOOK_FRAME_H_
#define JIL_BOOK_FRAME_H_
#pragma once

#include <list>
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
class Binding;
class FtPlugin;
class StatusBar;
class Style;
class TextBuffer;
class TextPoint;
class TextRange;
class TextWindow;
}  // namespace editor

class BookCtrl;
class BookPage;
class FindResultPage;
class FindWindow;
class Session;
class Splitter;
class SplitNode;
class TextBook;
class TextPage;
class ToolBook;

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

  // Toggle the wrap state of the active text page.
  void Wrap();
  // Toggle the line number show state of the active text page.
  void ShowNumber();
  // Toggle the white space show state of the active text page.
  void ShowSpace();

  // Show full screen or not.
  void FullScreen();

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

  // Find all in the given buffer, add the find result to the find result page.
  void FindAll(const std::wstring& str,
               editor::TextBuffer* buffer,
               int flags,
               FindResultPage* fr_page);

  // Find all in the given buffer, save the find result in each line.
  void FindAll(const std::wstring& str,
               editor::TextBuffer* buffer,
               int flags);

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
  void OnMenuFileRecentFile(wxCommandEvent& evt);
  void OnQuit(wxCommandEvent& evt);

  void OnMenuEdit(wxCommandEvent& evt);
  void OnMenuView(wxCommandEvent& evt);
  void OnMenuTools(wxCommandEvent& evt);
  void OnMenuHelp(wxCommandEvent& evt);

  // Return true if the command function is found and executed.
  bool ExecFuncByMenu(int menu);

  void OnFileUpdateUI(wxUpdateUIEvent& evt);
  void OnEditUpdateUI(wxUpdateUIEvent& evt);
  void OnViewUpdateUI(wxUpdateUIEvent& evt);

  void OnClose(wxCloseEvent& evt);

  // BookCtrl tab right-click menu event handler.
  void OnBookRClickMenu(wxCommandEvent& evt);

  // Text page(s) added or removed.
  void OnTextBookPageChange(wxCommandEvent& evt);

  // Active text page switch.
  void OnTextBookPageSwitch(wxCommandEvent& evt);

  // Tool page(s) added or removed.
  void OnToolBookPageChange(wxCommandEvent& evt);

  // Update status fields according to active text page.
  void UpdateStatusFields();

  // Update title according to active text page.
  void UpdateTitle();

  // Example: "3/120, 27"
  wxString FormatCaretString(TextPage* page) const;

  // Handle events from text window.
  void OnTextWindowEvent(wxCommandEvent& evt);

  // Handle text window get focus event.
  void HandleTextWindowGetFocus(wxCommandEvent& evt);

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

  // Get the page which has focus.
  BookPage* GetFocusedPage();

  // The focused page must be the current page, but the current page might
  // not be focused.
  // Only used by Save As.
  BookPage* GetCurrentPage();

private:
  // TODO: TextWindow also has AppendMenuItem, remove one of them.
  wxMenuItem* AppendMenuItem(wxMenu* menu,
                             int id,
                             const wxString& label,
                             wxItemKind kind = wxITEM_NORMAL);

  void LoadMenus();

  bool GetFileMenuState(int menu_id, wxString* text = NULL);
  bool GetEditMenuState(int menu_id);
  bool GetViewMenuState(int menu_id, bool* check = NULL);
  bool GetMenuEnableState(int menu_id);

  // Clear and recreate the items for Recent Files menu.
  void UpdateRecentFilesMenu();

  // Create text page with the given buffer.
  TextPage* CreateTextPage(editor::TextBuffer* buffer,
                           wxWindow* parent,
                           wxWindowID id = wxID_ANY);

  TextPage* TextPageByFileName(const wxFileName& fn_object) const;

  void RemovePage(const TextPage* page);
  void RemoveAllPages(const TextPage* except_page = NULL);

  void SwitchStackPage(bool forward);

  TextPage* DoOpenFile(const wxString& file_name,
                       bool active,
                       bool silent,
                       bool update_recent_files,
                       bool update_recent_files_menu);

  // \param silent Don't popup error message box on failure.
  // Return the text page of this file.
  TextPage* DoOpenFile(const wxFileName& fn_object,
                       bool active,
                       bool silent,
                       bool* new_opened = NULL);

  void DoSaveBuffer(editor::TextBuffer* buffer);

  void AddRecentFile(const wxString& recent_file);

private:
  Options* options_;
  Session* session_;

  // Splitter splits sub windows.
  Splitter* splitter_;

  TextBook* text_book_;
  ToolBook* tool_book_;

  // Current page type.
  // See BookPage::Page_Type().
  wxString page_type_;

  editor::StatusBar* status_bar_;

  editor::Style* style_;
  editor::SharedTheme theme_;

  editor::Binding* binding_;
  editor::Key leader_key_;

  // File menu -> Recent Files
  std::list<wxString> recent_files_;
  wxMenu* recent_files_menu_;

  // Screen rect before show full screen.
  wxRect last_screen_rect_;
};

}  // namespace jil

#endif  // JIL_BOOK_FRAME_H_
