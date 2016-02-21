#ifndef JIL_BOOK_FRAME_H_
#define JIL_BOOK_FRAME_H_
#pragma once

#include <list>
#include <vector>
#include "wx/frame.h"
#include "wx/arrstr.h"
#include "wx/filename.h"
#include "editor/compile_config.h"
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
class Options;
class Style;
class TextBuffer;
class TextPoint;
class TextRange;
class TextWindow;
}  // namespace editor

class BookCtrl;
class BookPage;
class FindResultPage;
class FindPanel;
class FindPanelEvent;
class Session;
class Splitter;
class SplitNode;
class StatusBar;
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

public:
  BookFrame(Options* options, editor::Options* editor_options, Session* session);
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

  // Restore last opened files from session.
  void RestoreOpenedFiles();

  // File menu operations.
  void FileNew();

#if JIL_MULTIPLE_WINDOW
  void FileNewWindow();
#endif  // JIL_MULTIPLE_WINDOW

  void FileOpen();

  void FileClose();
  void FileCloseAll();
  void FileCloseAllButThis();

  void FileSave();
  void FileSaveAs();
  void FileSaveAll();
  void FileCopyPath();
  void FileOpenFolder();

  size_t PageCount() const;

  void SwitchToNextPage();
  void SwitchToPrevPage();

  void SwitchToNextStackPage();
  void SwitchToPrevStackPage();

  void ShowFind();
  void ShowReplace();

  void FindNext();
  void FindPrev();

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

  void OnAbout(wxCommandEvent& evt);
  void ShowAboutWindow();

  void OnGlobalPreferences(wxCommandEvent& evt);

  // Compare global options and apply the changes.
  void ApplyGlobalOptionChanges(const Options& old_options);

  void ApplyLinePadding(int line_padding);
  void ApplyTextFont(const wxFont& font);
  void ApplyLineNrFont(const wxFont& font);

  void OnEditorPreferences(wxCommandEvent& evt);

  // Apply changes to the text pages/buffers of the given file type.
  void ApplyEditorOptionChanges(const wxString& ft_id,
                                const editor::Options& options,
                                const editor::Options& old_options);

  void OnTheme(wxCommandEvent& evt);

  void OnQuit(wxCommandEvent& evt);

  void OnMenuFile(wxCommandEvent& evt);
  void OnMenuFileRecentFile(wxCommandEvent& evt);

  void OnMenuEdit(wxCommandEvent& evt);
  void OnMenuView(wxCommandEvent& evt);
  void OnMenuTools(wxCommandEvent& evt);
  void OnMenuHelp(wxCommandEvent& evt);

  // Return true if the command function is found and executed.
  bool ExecFuncByMenu(int menu);

  void OnFileUpdateUI(wxUpdateUIEvent& evt);
  void OnEditUpdateUI(wxUpdateUIEvent& evt);
  void OnViewUpdateUI(wxUpdateUIEvent& evt);
  void OnThemeUpdateUI(wxUpdateUIEvent& evt);

  void OnClose(wxCloseEvent& evt);

  // Text page(s) added or removed.
  void OnTextBookPageChange(wxCommandEvent& evt);

  // Active text page switch.
  void OnTextBookPageSwitch(wxCommandEvent& evt);

  // Tool page(s) added or removed.
  void OnToolBookPageChange(wxCommandEvent& evt);

  // Update status fields according to active text page.
  void UpdateStatusFields();

  // Update status fields with the given text page.
  void UpdateStatusCaret(TextPage* page, bool refresh);
  void UpdateStatusTabOptions(TextPage* page, bool refresh);
  void UpdateStatusEncoding(TextPage* page, bool refresh);
  void UpdateStatusFileFormat(TextPage* page, bool refresh);
  void UpdateStatusFileType(TextPage* page, bool refresh);

  // Update title with the file path of the active text page.
  void UpdateTitleWithPath();

  // Example: "3/120, 27"
  wxString GetStatusCaretString(TextPage* page) const;
  // Example: "Tab: 4"
  wxString GetStatusTabOptionsString(TextPage* page) const;

  // Handle events from text window.
  void OnTextWindowEvent(wxCommandEvent& evt);

  // Handle text window get focus event.
  void HandleTextWindowGetFocus(wxCommandEvent& evt);

  // Handle events from find result page.
  void OnFindResultPageEvent(wxCommandEvent& evt);

  // Status field event handler.
  void OnStatusFieldClick(wxCommandEvent& evt);

  void PopupStatusTabOptionsMenu();
  void PopupStatusEncodingMenu();
  void PopupStatusFileFormatMenu();
  void PopupStatusFileTypeMenu();

  void OnStatusTabOptionsMenu(wxCommandEvent& evt);
  void OnStatusEncodingMenu(wxCommandEvent& evt);
  void OnStatusFileFormatMenu(wxCommandEvent& evt);
  void OnStatusFileTypeMenu(wxCommandEvent& evt);

  void UpdateStatusIndentOptionsField(TextPage* text_page);

  // Return find panel if it's shown.
  FindPanel* GetFindPanel() const;

  // \param mode See FindPanel::Mode
  void ShowFindPanel(int mode);

  // Close find panel, update layout, etc.
  void CloseFindPanel();

  // Handle find panel events.
  void OnFindPanelEvent(FindPanelEvent& evt);

  // Get find result page, create it if necessary.
  FindResultPage* GetFindResultPage(bool create);

  // Show tool book and activate the given page.
  void ActivateToolPage(BookPage* page);

  // Get the page which has focus.
  BookPage* GetFocusedPage();

  // The focused page must be the current page, but the current page might
  // not be focused.
  // Only used by Save As.
  BookPage* GetCurrentPage();

private:
  //----------------------------------------------------------------------------
  // Find & Replace

  void HandleFindStrChange(const std::wstring& str, int flags);
  void HandleFind(const std::wstring& str, int flags, FindLocation location);
  void HandleFindAll(const std::wstring& str, int flags, FindLocation location);

  // Find string in the active text page, select it and update the caret point.
  void FindInActivePage(const std::wstring& str, int flags);

  void FindInAllPages(const std::wstring& str, int flags);

  void FindAllInActivePage(const std::wstring& str, int flags);

  void FindAllInAllPages(const std::wstring& str, int flags);

  void ReplaceInActivePage(const std::wstring& str,
                           const std::wstring& replace_str,
                           int flags);

  void ReplaceAllInActivePage(const std::wstring& str,
                              const std::wstring& replace_str,
                              int flags);

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
  void FindAll(const std::wstring& str, editor::TextBuffer* buffer, int flags);

  // Select the result text range, update caret point and scroll to it if necessary.
  void SelectFindResult(TextPage* text_page, const editor::TextRange& result_range);

  // The find result won't be selected and the caret point won't be updated
  // if it's incremental.
  void SetFindResult(TextPage* text_page,
                     const editor::TextRange& find_result,
                     bool incremental);

  //----------------------------------------------------------------------------
  // Menu

  // TODO: TextWindow also has AppendMenuItem, remove one of them.
  wxMenuItem* AppendMenuItem(wxMenu* menu, int id, const wxString& label, wxItemKind kind = wxITEM_NORMAL);

  void LoadMenus();

  // Some void commands don't have menu items though they should always have menu IDs.
  // A void command without menu item can only be executed by shortcut keys.
  // When create menus, wxWidgets automatically builds and updates accelerator tables
  // with the shortcut keys specified in the menu's label. This is a bad design though
  // for most cases it's really convenient.
  // This function will overwrite the accelerator table built during creating menus.
  // TODO: Avoid building accelerator table when create menus.
  void SetAccelForVoidCmds();

  bool GetFileMenuState(int menu_id, wxString* text = NULL);
  bool GetEditMenuState(int menu_id);
  bool GetViewMenuState(int menu_id, bool* check = NULL);

#if JIL_ENABLE_LEADER_KEY
  bool GetMenuEnableState(int menu_id);
#endif  // JIL_ENABLE_LEADER_KEY

  // Add installed themes to the theme menu.
  void InitThemeMenu(wxMenu* theme_menu);

  void InitFileTypeMenu(wxMenu* ft_menu, int id_begin);

  // Clear and recreate the items for Recent Files menu.
  void UpdateRecentFilesMenu();

  //----------------------------------------------------------------------------
  // Text Page

  // Create text page with the given buffer.
  TextPage* CreateTextPage(editor::TextBuffer* buffer, wxWindow* parent, wxWindowID id);

  TextPage* TextPageByFileName(const wxFileName& fn_object) const;

  TextPage* TextPageByBufferId(size_t buffer_id) const;

  void RemoveAllPages(const TextPage* except_page = NULL);

  void SwitchStackPage(bool forward);

  //----------------------------------------------------------------------------
  // File - Open, Save

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
                       bool update_recent_files,
                       bool update_recent_files_menu);

  void DoSaveBuffer(editor::TextBuffer* buffer);

  //----------------------------------------------------------------------------

  void AddRecentFile(const wxString& recent_file, bool update_menu);
  void RemoveRecentFile(const wxString& recent_file, bool update_menu);

private:
  Options* options_;
  editor::Options* editor_options_;

  Session* session_;

  // Splitter splits sub windows.
  Splitter* splitter_;

  TextBook* text_book_;
  ToolBook* tool_book_;

  // Current page type.
  // See BookPage::Page_Type().
  wxString page_type_;

  StatusBar* status_bar_;

  FindPanel* find_panel_;

  std::wstring find_str_;
  int find_flags_;  // Find flags exluding kFind_Reversely.

  editor::Style* style_;
  editor::SharedTheme theme_;

  editor::Binding* binding_;

#if JIL_ENABLE_LEADER_KEY
  editor::Key leader_key_;
#endif  // JIL_ENABLE_LEADER_KEY

  // File menu -> Recent Files
  std::list<wxString> recent_files_;
  wxMenu* recent_files_menu_;

  // Screen rect before show full screen.
  wxRect last_screen_rect_;
};

}  // namespace jil

#endif  // JIL_BOOK_FRAME_H_
