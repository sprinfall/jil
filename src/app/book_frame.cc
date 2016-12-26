#include "app/book_frame.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <list>

#include "wx/clipbrd.h"
#include "wx/dataobj.h"
#include "wx/dir.h"
#include "wx/dnd.h"
#include "wx/filedlg.h"
#include "wx/image.h"
#include "wx/intl.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/msgdlg.h"
#include "wx/preferences.h"
#include "wx/sizer.h"
#include "wx/splitter.h"
#include "wx/toolbar.h"
#include "wx/stdpaths.h"
#include "wx/utils.h"

#include "base/string_util.h"

#include "editor/action.h"
#include "editor/util.h"
#include "editor/option.h"
#include "editor/lex.h"
#include "editor/key.h"
#include "editor/style.h"
#include "editor/text_buffer.h"
#include "editor/ft_plugin.h"
#include "editor/text_window.h"

#include "app/app.h"
#include "app/compile_config.h"
#include "app/config.h"
#include "app/defs.h"
#include "app/find_result_page.h"
#include "app/find_panel.h"
#include "app/find_thread.h"
#include "app/i18n_strings.h"
#include "app/navigation_dialog.h"
#include "app/page_window.h"
#include "app/save.h"
#include "app/session.h"
#include "app/splitter.h"
#include "app/status_bar.h"
#include "app/text_page.h"
#include "app/text_book.h"
#include "app/tool_book.h"
#include "app/util.h"

#include "app/pref/editor_dialog.h"
#include "app/pref/global_dialog.h"

namespace jil {

typedef std::list<editor::TextRange> TextRangeList;

static const editor::TextPoint kPointBegin(0, 1);

////////////////////////////////////////////////////////////////////////////////

// Drop files to book frame to open.
class FileDropTarget : public wxFileDropTarget {
 public:
  explicit FileDropTarget(BookFrame* book_frame)
      : book_frame_(book_frame) {
  }

  virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& file_names) override {
    book_frame_->OpenFiles(file_names, false);
    return true;
  }

 private:
  BookFrame* book_frame_;
};

////////////////////////////////////////////////////////////////////////////////

// Char hook event is actually a key down event, so we give it a better name.
#define EVT_KEYDOWN_HOOK EVT_CHAR_HOOK

BEGIN_EVENT_TABLE(BookFrame, wxFrame)
EVT_SIZE(BookFrame::OnSize)
#if JIL_MULTIPLE_WINDOW
EVT_ACTIVATE(BookFrame::OnActivate)
#endif  // JIL_MULTIPLE_WINDOW

EVT_KEYDOWN_HOOK(BookFrame::OnKeyDownHook)

EVT_MENU_RANGE(ID_MENU_FILE_BEGIN, ID_MENU_FILE_END - 1, BookFrame::OnMenuFile)
EVT_MENU_RANGE(ID_MENU_FILE_RECENT_FILE_0, ID_MENU_FILE_RECENT_FILE_9, BookFrame::OnMenuFileRecentFile)

EVT_MENU(wxID_ABOUT, BookFrame::OnAbout)
EVT_MENU(wxID_EXIT, BookFrame::OnQuit)

EVT_MENU_RANGE(ID_MENU_EDIT_BEGIN, ID_MENU_EDIT_END - 1, BookFrame::OnMenuEdit)
EVT_MENU_RANGE(ID_MENU_VIEW_BEGIN, ID_MENU_VIEW_END - 1, BookFrame::OnMenuView)
EVT_MENU_RANGE(ID_MENU_TOOLS_BEGIN, ID_MENU_TOOLS_END - 1, BookFrame::OnMenuTools)

EVT_MENU(wxID_PREFERENCES, BookFrame::OnGlobalPreferences)
EVT_MENU_RANGE(ID_MENU_PREFS_EDITOR_BEGIN, ID_MENU_PREFS_EDITOR_END - 1, BookFrame::OnEditorPreferences)
EVT_MENU_RANGE(ID_MENU_THEME_BEGIN, ID_MENU_THEME_END - 1, BookFrame::OnMenuTheme)

EVT_MENU_RANGE(ID_MENU_HELP_BEGIN, ID_MENU_HELP_END - 1, BookFrame::OnMenuHelp)

// Update UI
EVT_UPDATE_UI_RANGE(ID_MENU_FILE_BEGIN, ID_MENU_FILE_END - 1, BookFrame::OnFileUpdateUI)
EVT_UPDATE_UI_RANGE(ID_MENU_EDIT_BEGIN, ID_MENU_EDIT_END - 1, BookFrame::OnEditUpdateUI)
EVT_UPDATE_UI_RANGE(ID_MENU_VIEW_BEGIN, ID_MENU_VIEW_END - 1, BookFrame::OnViewUpdateUI)
EVT_UPDATE_UI_RANGE(ID_MENU_THEME_BEGIN, ID_MENU_THEME_END - 1, BookFrame::OnThemeUpdateUI)

EVT_CLOSE(BookFrame::OnClose)

// Use wxID_ANY to accept event from any text window.
EVT_TEXT_WINDOW(wxID_ANY, BookFrame::OnTextWindowEvent)

// Status bar.
EVT_STATUS_FIELD_CLICK(ID_STATUS_BAR, BookFrame::OnStatusFieldClick)
EVT_MENU_RANGE(ID_MENU_TAB_BEGIN, ID_MENU_TAB_END - 1, BookFrame::OnStatusTabOptionsMenu)
EVT_MENU_RANGE(ID_MENU_ENCODING_BEGIN, ID_MENU_ENCODING_END - 1, BookFrame::OnStatusEncodingMenu)
EVT_MENU_RANGE(ID_MENU_FILE_FORMAT_BEGIN, ID_MENU_FILE_FORMAT_END - 1, BookFrame::OnStatusFileFormatMenu)
EVT_MENU_RANGE(ID_MENU_FILE_TYPE_BEGIN, ID_MENU_FILE_TYPE_END - 1, BookFrame::OnStatusFileTypeMenu)

EVT_FIND_PANEL_LAYOUT(ID_FIND_PANEL, BookFrame::OnFindPanelLayoutEvent)

EVT_THREAD(ID_FIND_THREAD_EVENT, BookFrame::OnFindThreadEvent)

END_EVENT_TABLE()

BookFrame::BookFrame() {
  Init();
}

bool BookFrame::Create(wxWindow* parent, wxWindowID id, const wxString& title) {
  assert(options_ != NULL);
  assert(session_ != NULL);
  assert(theme_);
  assert(style_ != NULL);
  assert(binding_ != NULL);

  if (!wxFrame::Create(parent, id, title)) {
    return false;
  }

  recent_files_ = session_->recent_files();

  CreateFrBuffer();

  editor::SharedTheme bf_theme = theme_->GetTheme(THEME_BOOK_FRAME);

  splitter_ = new Splitter;
  splitter_->Create(this);
  splitter_->SetBackgroundColour(bf_theme->GetColor(COLOR_BG));

  // Create text book.
  text_book_ = new TextBook();
  text_book_->set_options(options_);
  text_book_->set_style(style_);
  text_book_->set_theme(theme_->GetTheme(THEME_TEXT_BOOK));
  text_book_->set_popup_theme(theme_->GetTheme(THEME_POPUP));
  text_book_->set_page_theme(theme_->GetTheme(THEME_TEXT_PAGE));
  text_book_->set_binding(binding_);
  text_book_->Create(splitter_, ID_TEXT_BOOK);
  text_book_->SetTabFont(options_->fonts[FONT_TABS]);

  Connect(text_book_->GetId(), kEvtBookPageChange, wxCommandEventHandler(BookFrame::OnTextBookPageChange));
  Connect(text_book_->GetId(), kEvtBookPageSwitch, wxCommandEventHandler(BookFrame::OnTextBookPageSwitch));

  // Create tool book.
  tool_book_ = new ToolBook();
  tool_book_->set_theme(theme_->GetTheme(THEME_TEXT_BOOK));
  tool_book_->set_popup_theme(theme_->GetTheme(THEME_POPUP));
  tool_book_->Create(splitter_, ID_TOOL_BOOK);
  tool_book_->SetTabFont(options_->fonts[FONT_TABS]);
  tool_book_->Hide();
  Connect(tool_book_->GetId(), kEvtBookPageChange, wxCommandEventHandler(BookFrame::OnToolBookPageChange));

  // Restore split tree from session or create it.
  SplitNode* split_root = session_->split_root();
  if (split_root != NULL) {
    RestoreSplitTree(split_root);
  } else {
    split_root = CreateDefaultSplitTree();
    session_->set_split_root(split_root);
  }
  splitter_->SetSplitRoot(split_root);

  // Create status bar.
  status_bar_ = new StatusBar;
  status_bar_->set_theme(theme_->GetTheme(THEME_STATUS_BAR));
  status_bar_->SetFields(wxGetApp().status_fields());

  status_bar_->Create(this, ID_STATUS_BAR);

  status_bar_->SetFont(options_->fonts[FONT_STATUS_BAR]);
  status_bar_->SetFieldValue(StatusBar::kField_Cwd, wxGetCwd(), false);

  LoadMenus();
  RecreateEditMenu();

  SetAccelForVoidCmds();

  SetDropTarget(new FileDropTarget(this));

  return true;
}

BookFrame::~BookFrame() {
  DeleteFrBuffer();
}

bool BookFrame::Destroy() {
  return wxFrame::Destroy();
}

bool BookFrame::Show(bool show) {
  // Restore last state.
#if defined (__WXGTK__)
  wxSize size = session_->book_frame_size();
  if (size.x <= 0 || size.y <= 0) {
    // Initial size: screen size * 0.75
    wxRect rect;
    wxClientDisplayRect(&rect.x, &rect.y, &rect.width, &rect.height);
    size = rect.GetSize() * 0.75;
  }
  // NOTE: In wxGTK, SetSize takes the size as client size. That's why we
  // save client size to session. See OnClose().
  // The x, y can't be 0. Using 1 moves the window to the top left of the
  // screen client area (not including the launcher and top panel).
  SetSize(1, 1, size.x, size.y);
#else
  wxRect rect = session_->book_frame_rect();
  if (rect.IsEmpty()) {
    // Initial size: screen size * 0.75
    wxClientDisplayRect(&rect.x, &rect.y, &rect.width, &rect.height);
    rect.Deflate(rect.width * 0.125, rect.height * 0.125);
  }
  SetSize(rect);
#endif  // __WXGTK__

  if (session_->book_frame_maximized()) {
    Maximize();
  }

  return wxFrame::Show(show);
}

TextPage* BookFrame::OpenFile(const wxString& file_name, bool active, bool silent) {
  return DoOpenFile(file_name, active, silent, true, true);
}

void BookFrame::OpenFiles(const wxArrayString& file_names, bool silent) {
  if (file_names.IsEmpty()) {
    return;
  }

  // KISS: Make the cursor busy instead of opening in another thread.
  wxBusyCursor busy_cursor;

  text_book_->StartBatch();

  bool active = true;
  for (size_t i = 0; i < file_names.size(); ++i) {
    // Don't update Recent Files menu for performance. Update it later.
    if (DoOpenFile(file_names[i], active, silent, true, false) != NULL) {
      active = false;  // Activate the first opened file.
    }
  }

  text_book_->EndBatch();

  UpdateRecentFilesMenu();
}

// Helper class for restoring last opened files.
class OpenedPage {
public:
  TextPage* page;
  int stack_index;
};

bool operator<(const OpenedPage& lhs, const OpenedPage& rhs) {
  return lhs.stack_index > rhs.stack_index;
}

// NOTE:
// Don't have to use wxBusyCursor. The cursor will be busy automatically (Why?).
void BookFrame::RestoreOpenedFiles() {
  const std::list<Session::OpenedFile>& opened_files = session_->opened_files();
  if (opened_files.empty()) {
    return;
  }

  // Update GUI. Otherwise, the tab area and the status bar won't be painted
  // when restore the last opened files.
  Update();

  text_book_->StartBatch();

  std::vector<OpenedPage> opened_pages;

  for (const Session::OpenedFile& opened_file : opened_files) {
    TextPage* text_page = DoOpenFile(opened_file.file_path,
                                     false,   // active: activate later according to stack index.
                                     true,    // silent: no warning message since the file might not exist any more.
                                     true,    // update_recent_files
                                     false);  // update_recent_files_menu: update later

    if (text_page != NULL) {
      OpenedPage opened_page = { text_page, opened_file.stack_index };
      opened_pages.push_back(opened_page);
    }
  }

  if (!opened_pages.empty()) {
    // Sort pages by stack index.
    std::sort(opened_pages.begin(), opened_pages.end());

    OpenedPage last_page = opened_pages.back();
    opened_pages.pop_back();

    // Adjust stack order.
    for (OpenedPage& opened_page : opened_pages) {
      text_book_->MovePageToStackFront(opened_page.page);
    }

    // NOTE: Visibility will be ensured in EndBatch().
    text_book_->ActivatePage(last_page.page, false);
  }

  text_book_->EndBatch();

  UpdateRecentFilesMenu();
}

void BookFrame::FileNew() {
  using namespace editor;

  // TODO: Let the user choose file type.
  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(kTxtFt);

  TextBuffer* buffer = new TextBuffer(NewBufferId(), ft_plugin, options_->file_encoding);

  text_book_->AddPage(buffer, true);
}

#if JIL_MULTIPLE_WINDOW
void BookFrame::FileNewWindow() {
  BookFrame* book_frame = new BookFrame(options_, session_);
  book_frame->set_theme(theme_);
  book_frame->set_style(style_);
  book_frame->set_binding(binding_);

  if (!book_frame->Create(NULL, wxID_ANY, kAppDisplayName)) {
    wxLogError(wxT("Failed to create book frame!"));
    return;
  }
  book_frame->Show();
}
#endif  // JIL_MULTIPLE_WINDOW

void BookFrame::FileOpen() {
  // TODO: wxFileDialog has memory leak!!!

  // NOTE: Don't use flag wxFD_CHANGE_DIR.
  int flags = wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_PREVIEW;

  wxFileDialog file_dialog(this,
                           wxFileSelectorPromptStr,
                           wxEmptyString,
                           wxEmptyString,
                           wxFileSelectorDefaultWildcardStr,
                           flags);

  if (file_dialog.ShowModal() != wxID_OK) {
    return;
  }

  wxArrayString file_paths;
  file_dialog.GetPaths(file_paths);

  if (!file_paths.empty()) {
    OpenFiles(file_paths, false);
  }
}

void BookFrame::FileClose() {
  BookPage* page = text_book_->ActivePage();
  if (page == NULL) {
    return;
  }

  // If the buffer is modified, ask for save.
  if (page->Page_IsModified()) {
    if (!page->Page_Save()) {
      return;  // Failed or canceled.
    }
  }

  text_book_->RemovePage(page);
}

void BookFrame::FileCloseAll() {
  RemoveAllTextPages(false, NULL);
}

void BookFrame::FileCloseAllButThis() {
  RemoveAllTextPages(false, text_book_->ActiveTextPage());
}

void BookFrame::FileSave() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer != NULL) {
    DoSaveBuffer(buffer);
  }
}

void BookFrame::FileSaveAs() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer == NULL) {
    return;
  }

  if (SaveBufferAs(buffer, this)) {
    AddRecentFile(buffer->file_path_name(), true);
  }
}

void BookFrame::FileSaveAll() {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    if (text_pages[i] != NULL) {
      DoSaveBuffer(text_pages[i]->buffer());
    }
  }
}

void BookFrame::FileCopyPath() {
  TextPage* page = text_book_->ActiveTextPage();
  if (page != NULL) {
    if (wxTheClipboard->Open()) {
      wxTheClipboard->SetData(new wxTextDataObject(page->buffer()->file_path_name()));
      wxTheClipboard->Close();
    }
  }
}

void BookFrame::FileOpenFolder() {
  TextPage* page = text_book_->ActiveTextPage();
  if (page != NULL) {
    ExploreFile(page->buffer()->file_path_name());
  }
}

//------------------------------------------------------------------------------

void BookFrame::SwitchToNextStackPage() {
  SwitchStackPage(true);
}

void BookFrame::SwitchToPrevStackPage() {
  SwitchStackPage(false);
}

void BookFrame::ShowFind() {
  ShowFindPanel(FindPanel::kFindMode);
}

void BookFrame::ShowReplace() {
  ShowFindPanel(FindPanel::kReplaceMode);
}

void BookFrame::FindNext() {
  if (!find_str_.empty()) {
    FindInActivePage(find_str_, find_flags_);
  }
}

void BookFrame::FindPrev() {
  if (!find_str_.empty()) {
    FindInActivePage(find_str_, find_flags_ | kFind_Reversely);
  }
}

void BookFrame::Wrap() {
  if (!text_book_->IsEmpty()) {
    PageWindow* page_window = text_book_->page_window();
    bool wrap = !page_window->view_options().wrap;
    page_window->Wrap(wrap);
  }
}

void BookFrame::ShowNumber() {
  if (!text_book_->IsEmpty()) {
    PageWindow* page_window = text_book_->page_window();
    bool show_number = !page_window->view_options().show_number;
    page_window->ShowNumber(show_number);
  }
}

void BookFrame::ShowSpace() {
  if (!text_book_->IsEmpty()) {
    PageWindow* page_window = text_book_->page_window();
    bool show_space = !page_window->view_options().show_space;
    page_window->ShowSpace(show_space);
  }
}

void BookFrame::FullScreen() {
  // Keep menu bar visible.
  long style = wxFULLSCREEN_NOTOOLBAR |
               wxFULLSCREEN_NOSTATUSBAR |
               wxFULLSCREEN_NOBORDER |
               wxFULLSCREEN_NOCAPTION;

  bool show = !IsFullScreen();

  // Save the screen rect for session in case exits from full screen mode.
  if (show) {
#if defined (__WXGTK__)
    last_client_size_ = GetClientSize();
#else
    last_screen_rect_ = GetScreenRect();
#endif  // __WXGTK__
  }

  ShowFullScreen(show, style);
}

TextPage* BookFrame::ActiveTextPage() const {
  return text_book_->ActiveTextPage();
}

editor::TextBuffer* BookFrame::ActiveBuffer() const {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return NULL;
  }
  return text_page->buffer();
}

//------------------------------------------------------------------------------
// Find & Replace

void BookFrame::SetLastFindStringAndFlags(const std::wstring& str, int flags) {
  find_str_ = str;

  // Remove "Reversely" flag.
  find_flags_ = flags & (~kFind_Reversely);
}

void BookFrame::FindInActivePageIncrementally(const std::wstring& str, int flags) {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  PageWindow* page_window = text_book_->page_window();

  if (str.empty()) {
    // Find string changes to empty, clear previous find result.
    page_window->ClearFindResult();
    return;
  }

  editor::TextPoint point = page_window->caret_point();

  if (!page_window->selection().IsEmpty()) {
    point = page_window->selection().begin();
  }

  editor::TextRange result_range = Find(text_page, str, point, flags, true);
  SetFindResult(page_window, result_range, true);
}

void BookFrame::FindInActivePage(const std::wstring& str, int flags) {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  PageWindow* page_window = text_book_->page_window();

  editor::TextPoint point = page_window->caret_point();

  if (GetBit(flags, kFind_Reversely)) {
    // If there's any selected text, it might be the previous find result. We
    // don't check it (for simplicity). But the find start point has to be
    // adjusted, otherwise the find result will always be this selected text.
    if (!page_window->selection().IsEmpty()) {
      point = page_window->selection().begin();
    }
  }

  editor::TextRange result_range = Find(text_page, str, point, flags, true);

  if (!result_range.IsEmpty()) {
    if (result_range == page_window->find_result() && !page_window->inc_find()) {
      // The find result is the same as last time.
      wxString msg_fmt = _("Can't find more '%s' in the current page.");
      wxString msg = wxString::Format(msg_fmt, wxString(str));
      ShowStatusMessage(msg, 3);
    }
  } else {
    wxString msg_fmt = _("Can't find '%s' in the current page.");
    wxString msg = wxString::Format(msg_fmt, wxString(str));
    ShowStatusMessage(msg, 3);
  }

  SetFindResult(page_window, result_range, false);
}

void BookFrame::FindAllInActivePage(const std::wstring& str, int flags) {
  wxBusyCursor busy;

  FindResultPage* fr_page = GetFindResultPage(true);
  ClearFindResult(fr_page);

  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL) {
    FindAll(str, flags, text_page->buffer(), true, fr_page->buffer());
  }
}

void BookFrame::FindAllInAllPages(const std::wstring& str, int flags) {
  wxBusyCursor busy;

  FindResultPage* fr_page = GetFindResultPage(true);
  ClearFindResult(fr_page);

  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    FindAll(str, flags, text_pages[i]->buffer(), true, fr_page->buffer());
  }
}

void BookFrame::FindAllInFolders(const std::wstring& str,
                                 int flags,
                                 const wxArrayString& folders) {
  wxCriticalSectionLocker locker(find_thread_cs_);

  if (find_thread_ != NULL) {
    wxLogDebug(wxT("Find thread is still running."));
    return;
  }

  FindResultPage* fr_page = GetFindResultPage(true);
  ClearFindResult(fr_page);

  wxArrayString files;

  size_t folders_count = folders.GetCount();
  for (size_t i = 0; i < folders_count; ++i) {
    wxString folder = folders[i];

    if (wxDir::Exists(folder)) {
      wxDir::GetAllFiles(folder, &files, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
    }
  }

  if (files.IsEmpty()) {
    return;
  }

  find_thread_ = new FindThread(this);
  find_thread_->set_str(str);
  find_thread_->set_flags(flags);
  find_thread_->set_files(files);

  if (find_thread_->Run() != wxTHREAD_NO_ERROR) {
    wxDELETE(find_thread_);
  }
}

int BookFrame::AsyncFindInFile(const std::wstring& str,
                               int flags,
                               const wxString& file) {
  editor::Coord old_line_count = fr_buffer_->LineCount();

  wxFileName fn(file);
  TextPage* text_page = TextPageByFileName(fn);

  if (text_page != NULL) {
    FindAll(str, flags, text_page->buffer(), false, fr_buffer_);
  } else {
    // Load the file to buffer without scanning lex.
    editor::TextBuffer* buffer = CreateBuffer(fn, kTempBufferId, false);
    if (buffer != NULL) {
      FindAll(str, flags, buffer, false, fr_buffer_);
      wxDELETE(buffer);
    }
  }

  return fr_buffer_->LineCount() - old_line_count;
}

void BookFrame::ReplaceInActivePage(const std::wstring& str,
                                    const std::wstring& replace_str,
                                    int flags) {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  PageWindow* page_window = text_book_->page_window();

  // Find start point.
  editor::TextPoint point = page_window->caret_point();

  // If there's any selection, the start point has to be adjusted to
  // the begin point of the selection.
  const editor::Selection& selection = page_window->selection();
  if (!selection.IsEmpty()) {
    point = selection.begin();
  }

  editor::TextRange result_range = Find(text_page, str, point, flags, true);
  if (result_range.IsEmpty()) {
    ShowStatusMessage(kTrNoMatching, 1);
    return;
  }

  page_window->ClearSelection();

  bool grouped = !replace_str.empty();

  page_window->DeleteRange(result_range, editor::kForward, false, grouped, false, false);

  if (!replace_str.empty()) {
    page_window->InsertString(result_range.point_begin(), replace_str, grouped, false);
  }

  // Go to next match.
  point = page_window->caret_point();
  result_range = Find(text_page, str, point, flags, true);
  SetFindResult(page_window, result_range, false);
}

void BookFrame::ReplaceAllInActivePage(const std::wstring& str,
                                       const std::wstring& replace_str,
                                       int flags) {
  using namespace editor;

  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  wxBusyCursor busy;

  ReplaceAll(str, replace_str, flags, text_page);
}

void BookFrame::ReplaceAllInAllPages(const std::wstring& str,
                                     const std::wstring& replace_str,
                                     int flags) {
  wxBusyCursor busy;

  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    ReplaceAll(str, replace_str, flags, text_pages[i]);
  }
}

// TODO
void BookFrame::ReplaceAllInFolders(const std::wstring& str,
                                    const std::wstring& replace_str,
                                    int flags,
                                    const wxArrayString& folders) {
}

//------------------------------------------------------------------------------

void BookFrame::Init() {
  options_ = NULL;
  editor_options_ = NULL;
  session_ = NULL;

  style_ = NULL;
  binding_ = NULL;

  splitter_ = NULL;

  text_book_ = NULL;
  tool_book_ = NULL;

  status_bar_ = NULL;

  find_panel_ = NULL;

  fr_buffer_ = NULL;
  find_flags_ = 0;
  find_thread_ = NULL;

  edit_menu_ = NULL;
  recent_files_menu_ = NULL;
}

//------------------------------------------------------------------------------

void BookFrame::OnSize(wxSizeEvent& evt) {
  if (GetClientSize().x < editor::kUnreadyWindowSize) {
    return;
  }

  UpdateLayout();
}

void BookFrame::UpdateLayout() {
  wxRect client_rect = GetClientRect();

  int find_panel_height = 0;
  if (find_panel_ != NULL) {
    find_panel_height = find_panel_->GetBestSize().y;
  }

  int status_height = status_bar_->GetBestSize().y;

  int book_height = client_rect.height - find_panel_height - status_height;

  splitter_->SetSize(0, 0, client_rect.width, book_height);
  splitter_->Split();

  int y = book_height;

  if (find_panel_ != NULL) {
    find_panel_->SetSize(0, y, client_rect.width, find_panel_height);
    y += find_panel_height;
  }

  status_bar_->SetSize(0, y, client_rect.width, status_height);
}

void BookFrame::RestoreSplitTree(SplitNode* n) {
  SplitLeaf* leaf = n->AsLeaf();
  if (leaf != NULL) {
    if (leaf->id() == kSplit_Text) {
      leaf->set_window(text_book_);
    } else if (leaf->id() == kSplit_Tool) {
      leaf->set_window(tool_book_);
    }
    return;
  }

  RestoreSplitTree(n->node1());
  RestoreSplitTree(n->node2());
}

SplitNode* BookFrame::CreateDefaultSplitTree() {
  SplitNode* split_root = new SplitNode(wxHORIZONTAL, 0.5f);

  SplitLeaf* text_node = new SplitLeaf(text_book_);
  text_node->set_id(kSplit_Text);
  split_root->set_node1(text_node);

  SplitLeaf* tool_node = new SplitLeaf(tool_book_);
  tool_node->set_id(kSplit_Tool);
  split_root->set_node2(tool_node);

  return split_root;
}

#if JIL_MULTIPLE_WINDOW
// Set current active book frame as the top window.
// TODO: Match command func to execute for commands not in the menu.
void BookFrame::OnActivate(wxActivateEvent& evt) {
  if (evt.GetActive()) {
    if (this != wxGetApp().GetTopWindow()) {
      wxGetApp().SetTopWindow(this);
    }
  }
  evt.Skip();
}
#endif  // JIL_MULTIPLE_WINDOW

void BookFrame::OnKeyDownHook(wxKeyEvent& evt) {
  if (!HandleKeyDownHook(evt)) {
    // Skip for child windows, e.g., TextWindow.
    evt.Skip();
  }
}

bool BookFrame::HandleKeyDownHook(wxKeyEvent& evt) {
  int code = evt.GetKeyCode();

#if !JIL_ENABLE_LEADER_KEY

  // Close find panel on ESC key.
  if (code == WXK_ESCAPE && find_panel_ != NULL) {
    CloseFindPanel();
    return true;
  }

  // Return false to let current text window clear selection.
  return false;

#else
  // Leader key is enabled.

  // NOTE: Don't match text command because you don't know what the current
  // text window is. Instead, return false and let the system delegate key down
  // event to the current text window.

  int modifiers = evt.GetModifiers();

  if (code == WXK_NONE) {
    return false;
  }
  if (code == WXK_CONTROL || code == WXK_SHIFT || code == WXK_ALT) {
    return false;
  }

  if (code == WXK_ESCAPE) {
    if (!leader_key_.IsEmpty()) {
      // Clear leader key in the status bar.
      leader_key_.Reset();
      status_bar_->SetFieldValue(editor::StatusBar::kField_KeyStroke, wxEmptyString, true);
      return true;
    } else if (find_panel_ != NULL) {
      CloseFindPanel();
      return true;
    }

    // Return false to let current text window clear selection.
    return false;
  }

  if (leader_key_.IsEmpty() && modifiers != 0) {
    if (binding_->IsLeaderKey(editor::Key(code, modifiers))) {
      leader_key_.Set(code, modifiers);

      status_bar_->SetFieldValue(editor::StatusBar::kField_KeyStroke,
                                 leader_key_.ToString() + wxT(", ..."),
                                 true);
      return true;
    }
  }

  if (code == WXK_TAB && modifiers == 0 && leader_key_.IsEmpty()) {
    // Let current text window input tab.
    return false;
  }

  // Avoid matching for single character key.
  if (leader_key_.IsEmpty()) {
    if (modifiers == 0) {
      // Standard ASCII characters || ASCII extended characters.
      if (code >= 33 && code <= 126 || code >= 128 && code <= 255) {
        return false;
      }
    } else if (modifiers == wxMOD_SHIFT && code < 127) {
      if (editor::kNonShiftChars.find(static_cast<char>(code)) != std::string::npos) {
        // ~!@#$%^&*()_+<>?:"{}|
        return false;
      }
    }
  }

  // Create the key.
  editor::Key key(code, modifiers);
  if (!leader_key_.IsEmpty()) {
    key.set_leader(leader_key_);
  }

  // Match void command.
  int menu = 0;
  editor::VoidFunc* void_func = binding_->GetVoidFuncByKey(key, &menu);
  if (void_func != NULL) {
    // Clear leader key before execute the function.
    if (!leader_key_.IsEmpty()) {
      leader_key_.Reset();
      status_bar_->SetFieldValue(editor::StatusBar::kField_KeyStroke, wxEmptyString, true);
    }

    // If the command has a menu, execute it only when it's enabled.
    if (menu == 0 || GetMenuEnableState(menu)) {
      void_func->Exec();
    }

    return true;
  }

  // No void command matched.
  // Return false to let current focused window handle the event.

  // If a text command is matched in the focused text window, the leader key
  // will also be reset if it's not empty (An event will be posted from the
  // text window for this). See OnTextWindowEvent().

  // TODO: How to check text menu's enable state in TextWindow?

  return false;

#endif  // !JIL_ENABLE_LEADER_KEY
}

void BookFrame::OnMenuFile(wxCommandEvent& evt) {
  // Find menu has no menu items mapping to text function.
  // So only search for void function.
  int menu = evt.GetId();

  editor::VoidFunc* void_func = binding_->GetVoidFuncByMenu(menu);
  if (void_func != NULL) {
    void_func->Exec();
  }
}

void BookFrame::OnMenuFileRecentFile(wxCommandEvent& evt) {
  size_t i = evt.GetId() - ID_MENU_FILE_RECENT_FILE_0;
  if (i >= recent_files_.size()) {
    return;
  }

  std::list<wxString>::iterator it = recent_files_.begin();
  std::advance(it, i);
  wxString file = *it;

  if (DoOpenFile(file, true, false, true, true) == NULL) {
    // Failed to open the file, remove it from the Recent Files list.
    RemoveRecentFile(file, true);
  }
}

void BookFrame::OnAbout(wxCommandEvent& WXUNUSED(evt)) {
  ShowAboutWindow();
}

void BookFrame::ShowAboutWindow() {
  wxMessageBox(kTrComingSoon);
}

void BookFrame::OnGlobalPreferences(wxCommandEvent& WXUNUSED(evt)) {
  // Backup current options.
  Options old_options = *options_;

  pref::GlobalDialog dialog(options_);
  dialog.Create(this, wxID_ANY, kTrOptions);
  dialog.CenterOnParent();

  if (dialog.ShowModal() != wxID_OK) {
    return;
  }

  // Apply changes.
  ApplyGlobalOptionChanges(old_options);

  // Save options file.
  wxGetApp().SaveUserGlobalOptions();
}

void BookFrame::ApplyGlobalOptionChanges(const Options& old_options) {
  if (options_->show_path != old_options.show_path) {
    if (options_->show_path) {
      UpdateTitleWithPath();
    } else {
      SetTitle(kAppDisplayName);
    }
  }

  bool need_update_layout = false;

  if (options_->line_padding != old_options.line_padding) {
    ApplyLinePadding(options_->line_padding);
  }

  if (options_->fonts[FONT_TEXT] != old_options.fonts[FONT_TEXT]) {
    ApplyTextFont(options_->fonts[FONT_TEXT]);
  }

  if (options_->fonts[FONT_TABS] != old_options.fonts[FONT_TABS]) {
    text_book_->SetTabFont(options_->fonts[FONT_TABS]);
    tool_book_->SetTabFont(options_->fonts[FONT_TABS]);
  }

  if (options_->fonts[FONT_STATUS_BAR] != old_options.fonts[FONT_STATUS_BAR]) {
    status_bar_->SetFont(options_->fonts[FONT_STATUS_BAR]);
    need_update_layout = true;
  }

  if (find_panel_ != NULL) {
    if (options_->fonts[FONT_FIND_PANEL] != old_options.fonts[FONT_FIND_PANEL]) {
      find_panel_->SetFont(options_->fonts[FONT_FIND_PANEL]);
      need_update_layout = true;
    }
  }

  if (options_->theme != old_options.theme ||
      options_->icon_resolution != old_options.icon_resolution) {
    ReloadTheme(options_->theme);
  }

  if (need_update_layout) {
    UpdateLayout();
  }
}

void BookFrame::ApplyLinePadding(int line_padding) {
  text_book_->page_window()->SetLinePadding(line_padding);

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page != NULL) {
    fr_page->SetLinePadding(line_padding);
  }
}

void BookFrame::ApplyTextFont(const wxFont& font) {
  text_book_->page_window()->SetTextFont(font);

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page != NULL) {
    fr_page->SetTextFont(font);
  }
}

void BookFrame::OnEditorPreferences(wxCommandEvent& evt) {
  App& app = wxGetApp();

  int index = evt.GetId() - ID_MENU_PREFS_EDITOR_BEGIN;
  if (index < 0 || index >= app.GetFileTypeCount()) {
    return;
  }

  const editor::FileType* ft = app.GetFileType(index);
  editor::FtPlugin* ft_plugin = app.GetFtPlugin(*ft);

  // Backup the old options.
  editor::Options old_options = ft_plugin->options();

  // This will be the new options.
  editor::Options options = ft_plugin->options();

  wxString title = kTrOptions + wxT(" - ") + ft->name;
  pref::EditorDialog dialog(&options);
  dialog.Create(this, wxID_ANY, title);
  dialog.CenterOnParent();

  if (dialog.ShowModal() != wxID_OK) {
    return;
  }

  ft_plugin->set_options(options);

  // Save options file.
  app.SaveUserEditorOptions(ft_plugin->id(), options);

  // Apply changes to this file type.
  ApplyEditorOptionChanges(ft->id, options, old_options);
}

void BookFrame::ApplyEditorOptionChanges(const wxString& ft_id,
                                         const editor::Options& options,
                                         const editor::Options& old_options) {
  bool view_options_changed = options.view != old_options.view;

  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (TextPage* text_page : text_pages) {
    editor::TextBuffer* buffer = text_page->buffer();

    if (buffer->ft_plugin()->id() != ft_id) {
      continue;
    }

    // Apply text option changes.

    buffer->set_text_options(options.text);

    if (options.text.tab_stop != old_options.text.tab_stop ||
        options.text.expand_tab != old_options.text.expand_tab) {
      // Notify to update the copy in text window and the status field.
      buffer->Notify(editor::kTabOptionsChange);
    }

    // Apply view option changes.

    // TODO
    if (view_options_changed) {
      buffer->set_view_options(options.view);

    //  if (options.view.wrap != old_options.view.wrap) {
    //    text_page->Wrap(options.view.wrap);
    //  }

      //if (options.view.show_number != old_options.view.show_number) {
      //  text_page->ShowNumber(options.view.show_number);
      //}

    //  if (options.view.show_space != old_options.view.show_space) {
    //    text_page->ShowSpace(options.view.show_space);
    //  }

    //  if (options.view.show_hscrollbar != old_options.view.show_hscrollbar) {
    //    text_page->ShowHScrollbar(options.view.show_hscrollbar);
    //  }

    //  if (options.view.rulers != old_options.view.rulers) {
    //    text_page->SetRulers(options.view.rulers);
    //  }
    }
  }
}

void BookFrame::OnMenuTheme(wxCommandEvent& evt) {
  App& app = wxGetApp();

  int index = evt.GetId() - ID_MENU_THEME_BEGIN;
  wxString theme_name = app.GetTheme(index);
  if (theme_name != options_->theme) {
    return;
  }

  options_->theme = theme_name;
  app.SaveUserGlobalOptions();

  ReloadTheme(theme_name);
}

void BookFrame::ReloadTheme(const wxString& theme_name) {
  App& app = wxGetApp();

  if (!app.ReloadTheme(theme_name)) {
    return;
  }

  // Apply theme.

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page != NULL) {
    fr_page->ReapplyTheme();
  }

  text_book_->ReapplyTheme();
  text_book_->Refresh();

  tool_book_->ReapplyTheme();
  if (tool_book_->IsShown()) {
    tool_book_->Refresh();
  }

  status_bar_->Refresh();

  if (find_panel_ != NULL) {
    find_panel_->ReapplyTheme();
    find_panel_->UpdateLayout();
  }
}

void BookFrame::OnQuit(wxCommandEvent& WXUNUSED(evt)) {
  Close(true);  // Go to OnClose().
}

//------------------------------------------------------------------------------

void BookFrame::OnMenuEdit(wxCommandEvent& evt) {
  ExecFuncByMenu(evt.GetId());
}

//------------------------------------------------------------------------------

void BookFrame::OnMenuView(wxCommandEvent& evt) {
  // View menu has no menu items mapping to text function.
  // So only search for void function.
  int menu = evt.GetId();
  editor::VoidFunc* void_func = binding_->GetVoidFuncByMenu(menu);
  if (void_func != NULL) {
    void_func->Exec();
  }
}

//------------------------------------------------------------------------------

void BookFrame::OnMenuTools(wxCommandEvent& evt) {
}

//------------------------------------------------------------------------------

void BookFrame::OnMenuHelp(wxCommandEvent& evt) {
  switch (evt.GetId()) {
    case ID_MENU_HELP_VIEW_ONLINE:
      wxMessageBox(kTrComingSoon);
      break;
  }
}

//------------------------------------------------------------------------------

bool BookFrame::ExecFuncByMenu(int menu) {
  // Delegate to the current page to handle it.
  BookPage* page = GetCurrentPage();
  if (page != NULL) {
    if (page->Page_OnMenu(menu)) {
      return true;
    }
  }

  // Match void command.
  editor::VoidFunc* void_func = binding_->GetVoidFuncByMenu(menu);
  if (void_func != NULL) {
    void_func->Exec();
    return true;
  }

  // No command matched.
  return false;
}

//------------------------------------------------------------------------------

void BookFrame::OnFileUpdateUI(wxUpdateUIEvent& evt) {
  evt.Enable(GetFileMenuState(evt.GetId()));
}

void BookFrame::OnEditUpdateUI(wxUpdateUIEvent& evt) {
  int menu_id = evt.GetId();
  bool state = GetEditMenuState(menu_id);
  evt.Enable(state);
}

void BookFrame::OnViewUpdateUI(wxUpdateUIEvent& evt) {
  bool check = false;
  bool state = GetViewMenuState(evt.GetId(), &check);
  evt.Enable(state);
  evt.Check(check);
}

void BookFrame::OnThemeUpdateUI(wxUpdateUIEvent& evt) {
  int index = evt.GetId() - ID_MENU_THEME_BEGIN;
  wxString theme = wxGetApp().GetTheme(index);
  evt.Check(theme == options_->theme);
}

// Close process:
// - BookFrame::OnClose()
//   - TextBook closes all text pages.
//   - ToolBook closes all tool pages (find result page, etc.)
//     - FindResultPage::Page_Close()
//       - Destroy()
//       - FindResultPage::~FindResultPage()
//       - TextWindow::~TextWindow()
void BookFrame::OnClose(wxCloseEvent& evt) {
  StopFindThread();

  // Remember opened files.
  session_->ClearOpenedFiles();
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    if (!text_pages[i]->buffer()->new_created()) {
      wxString file_path = text_pages[i]->buffer()->file_path_name();
      int stack_index = text_book_->GetStackIndex(text_pages[i]);
      session_->AddOpenedFile(file_path, stack_index);
    }
  }

  RemoveAllTextPages(true, NULL);

  if (text_book_->PageCount() > 0) {
    return;
  }

  session_->set_recent_files(recent_files_);

  if (IsMaximized()) {
    session_->set_book_frame_maximized(true);
  } else {
    session_->set_book_frame_maximized(false);

    if (IsFullScreen()) {
#if defined (__WXGTK__)
      session_->set_book_frame_size(last_client_size_);
#else
      session_->set_book_frame_rect(last_screen_rect_);
#endif  // __WXGTK__
    } else {
#if defined (__WXGTK__)
      // NOTE: In wxGTK, what SetSize accepts is actually the client size. So
      // we save client size to session.
      session_->set_book_frame_size(GetClientSize());
#else
      session_->set_book_frame_rect(GetScreenRect());
#endif  // __WXGTK__
    }
  }

  if (find_panel_ != NULL) {
    session_->set_find_flags(find_panel_->flags());
    session_->set_find_location(find_panel_->location());
  }

  // Explicitly remove all pages from tool book to avoid destroy order issue.
  tool_book_->RemoveAllPages(true, NULL);

  // Skip so that book frame can be destroyed.
  evt.Skip();
}

void BookFrame::OnTextBookPageChange(wxCommandEvent& evt) {
  // Clear status fields if all pages are removed.
  if (text_book_->PageCount() == 0) {
    UpdateStatusFields();

    if (options_->show_path) {
      UpdateTitleWithPath();
    }

    // Transfer focus to tool book if it's shown.
    if (tool_book_->IsShown()) {
      tool_book_->SetFocus();
    }

    // Close find panel.
    FindPanel* find_panel = GetFindPanel();
    if (find_panel != NULL) {
      find_panel->Close();
    }
  }
}

void BookFrame::OnTextBookPageSwitch(wxCommandEvent& evt) {
  UpdateStatusFields();

  if (options_->show_path) {
    UpdateTitleWithPath();
  }
}

void BookFrame::OnToolBookPageChange(wxCommandEvent& evt) {
  // If tool book has no page, hide it.
  if (tool_book_->PageCount() == 0) {
    if (tool_book_->IsShown()) {
      tool_book_->Hide();
      splitter_->Split();  // Update layout.

      // Transfer focus to text book.
      text_book_->SetFocus();
    }
  }
}

void BookFrame::UpdateStatusFields() {
  if (text_book_->IsEmpty()) {
    status_bar_->ClearFieldValues();
  } else {
    PageWindow* page_window = text_book_->page_window();

    // Update field values.

    editor::TextBuffer* buffer = page_window->buffer();

    if (options_->switch_cwd && !buffer->new_created()) {
      if (status_bar_->HasField(StatusBar::kField_Cwd)) {
        wxString cwd = buffer->file_path(wxPATH_GET_VOLUME, wxPATH_NATIVE);
        wxSetWorkingDirectory(cwd);
        status_bar_->SetFieldValue(StatusBar::kField_Cwd, cwd, false);
      }
    }

    UpdateStatusCaret(page_window, false);
    UpdateStatusTabOptions(page_window, false);
    UpdateStatusEncoding(page_window, false);
    UpdateStatusFileFormat(page_window, false);
    UpdateStatusFileType(page_window, false);
  }

  status_bar_->UpdateFieldSizes();
  status_bar_->Refresh();
}

void BookFrame::UpdateStatusCaret(PageWindow* pw, bool refresh) {
  StatusBar::FieldId field_id = StatusBar::kField_Caret;
  if (status_bar_->HasField(field_id)) {
    wxString field_value = GetStatusCaretString(pw);
    status_bar_->SetFieldValue(field_id, field_value, refresh);
  }
}

void BookFrame::UpdateStatusTabOptions(PageWindow* pw, bool refresh) {
  StatusBar::FieldId field_id = StatusBar::kField_TabOptions;
  if (status_bar_->HasField(field_id)) {
    wxString field_value = GetStatusTabOptionsString(pw);
    status_bar_->SetFieldValue(field_id, field_value, refresh);
  }
}

void BookFrame::UpdateStatusEncoding(PageWindow* pw, bool refresh) {
  StatusBar::FieldId field_id = StatusBar::kField_Encoding;
  if (status_bar_->HasField(field_id)) {
    wxString field_value = pw->buffer()->file_encoding().display_name;
    status_bar_->SetFieldValue(field_id, field_value, refresh);
  }
}

void BookFrame::UpdateStatusFileFormat(PageWindow* pw, bool refresh) {
  StatusBar::FieldId field_id = StatusBar::kField_FileFormat;
  if (status_bar_->HasField(field_id)) {
    wxString field_value = GetFileFormatName(pw->buffer()->file_format());
    status_bar_->SetFieldValue(field_id, field_value, refresh);
  }
}

void BookFrame::UpdateStatusFileType(PageWindow* pw, bool refresh) {
  StatusBar::FieldId field_id = StatusBar::kField_FileType;
  if (status_bar_->HasField(field_id)) {
    wxString field_value = pw->buffer()->ft_plugin()->name();
    status_bar_->SetFieldValue(field_id, field_value, refresh);
  }
}

void BookFrame::UpdateTitleWithPath() {
  TextPage* text_page = text_book_->ActiveTextPage();
  if (text_page == NULL) {
    SetTitle(kAppDisplayName);
  } else {
    wxString title = text_page->Page_Description() + wxT(" - ") + kAppDisplayName;
    SetTitle(title);
  }
}

wxString BookFrame::GetStatusCaretString(PageWindow* pw) const {
  editor::TextPoint point = pw->caret_point();
  wxString format = kTrLine + wxT(" %d, ") + kTrColumn + wxT(" %d");
  return wxString::Format(format, point.y, point.x);
}

wxString BookFrame::GetStatusTabOptionsString(PageWindow* pw) const {
  wxString indent_str = pw->expand_tab() ? _("Space") : _("Tab");
  indent_str += wxString::Format(wxT(" - %d"), pw->tab_stop());
  return indent_str;
}

// Update status bar according to the event.
void BookFrame::OnTextWindowEvent(wxCommandEvent& evt) {
  using namespace editor;

  int type = evt.GetInt();

  if (type == TextWindow::kGetFocusEvent) {
    // NOTE: Don't try to get the focused page from evt.GetEventObject().
    // A window could be destroyed immediately after it gets focus.
    RecreateEditMenu();
    return;
  }

#if JIL_ENABLE_LEADER_KEY
  if (type == TextWindow::kLeaderKeyEvent) {
    // Leader key is reset by the text window.
    if (leader_key_.IsEmpty()) {  // Must be empty but just check it.
      status_bar_->SetFieldValue(StatusBar::kField_KeyStroke, wxEmptyString, true);
    }
  }
#endif  // JIL_ENABLE_LEADER_KEY

  if (text_book_->IsEmpty()) {
    return;
  }

  PageWindow* page_window = text_book_->page_window();

  // Caret event is very frequently, so avoid updating field sizes.
  if (type == TextWindow::kCaretEvent) {
    UpdateStatusCaret(page_window, true);
    return;
  }

  if (type == TextWindow::kEncodingEvent) {
    UpdateStatusEncoding(page_window, false);
  } else if (type == TextWindow::kFileTypeEvent) {
    UpdateStatusFileType(page_window, false);
  } else if (type == TextWindow::kFileFormatEvent) {
    UpdateStatusFileFormat(page_window, false);
  } else if (type == TextWindow::kTabOptionsEvent) {
    UpdateStatusTabOptions(page_window, false);
  }

  status_bar_->UpdateFieldSizes();
  status_bar_->Refresh();
}

void BookFrame::RecreateEditMenu() {
  if (edit_menu_ == NULL) {
    return;
  }

  BookPage* current_page = GetCurrentPage();
  if (current_page == NULL) {
    current_page = text_book_->placeholder_page();
  }

  if (current_page != NULL) {
    int page_type = current_page->Page_Type();
    if (page_type != page_type_) {
      page_type_ = page_type;
      ClearMenuItems(edit_menu_);
      current_page->Page_EditMenu(edit_menu_);
    }
  }
}

void BookFrame::OnFindResultPageEvent(wxCommandEvent& evt) {
  int type = evt.GetInt();

  if (type == FindResultPage::kLocalizeEvent) {
    FindResultPage* fr_page = GetFindResultPage(false);
    if (fr_page != NULL) {
      HandleFindResultPageLocalize(fr_page);
    }
  } else if (type == FindResultPage::kDestroyEvent) {
    HandleFindResultPageDestroy();
  }
}

void BookFrame::HandleFindResultPageLocalize(FindResultPage* fr_page) {
  using namespace editor;

  TextBuffer* fr_buffer = fr_page->buffer();
  TextLine* fr_line = fr_buffer->Line(fr_page->caret_point().y);

  const boost::any& any = fr_line->extra_data();
  if (any.empty()) {
    return;
  }

  FrExtraData fr_extra_data;
  try {
    fr_extra_data = boost::any_cast<FrExtraData>(any);
  } catch (boost::bad_any_cast&) {
    return;
  }

  TextPage* text_page = NULL;

  if (!fr_extra_data.file_path.IsEmpty()) {
    wxFileName fn(fr_extra_data.file_path);
    text_page = TextPageByFileName(fn);

    if (text_page != NULL) {
      // The page might not be active, activate it.
      text_book_->ActivatePage(text_page, true);
      text_book_->SetFocus();
    } else {
      // The page is closed, reopen it.
      text_page = DoOpenFile(fn, true, false, true, true);
    }
  } else if (fr_extra_data.buffer_id) {
    // The buffer has no file path, a new buffer.
    text_page = TextPageByBufferId(fr_extra_data.buffer_id);

    if (text_page != NULL) {
      // The page might not be active, activate it.
      text_book_->ActivatePage(text_page, true);
      text_book_->SetFocus();
    }
  }

  if (text_page == NULL) {
    return;
  }

  // Go to the source line.
  Coord ln = text_page->buffer()->LineNrFromId(fr_extra_data.line_id);
  if (ln != kInvCoord) {
    text_book_->page_window()->Goto(ln);
  }
}

void BookFrame::HandleFindResultPageDestroy() {
  StopFindThread();
}

void BookFrame::OnStatusFieldClick(wxCommandEvent& evt) {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer == NULL) {
    return;
  }

  int field_id = evt.GetInt();

  // ISSUE (GTK):
  // The popup menu's background color doesn't follow the system theme.
  // The menu bar doesn't have this issue.

  switch (field_id) {
    case StatusBar::kField_TabOptions:
      PopupStatusTabOptionsMenu();
      break;

    case StatusBar::kField_Encoding:
      PopupStatusEncodingMenu();
      break;

    case StatusBar::kField_FileFormat:
      PopupStatusFileFormatMenu();
      break;

    case StatusBar::kField_FileType:
      PopupStatusFileTypeMenu();
      break;

    default:
      break;
  }
}

void BookFrame::PopupStatusTabOptionsMenu() {
  if (text_book_->IsEmpty()) {
    return;
  }

  PageWindow* page_window = text_book_->page_window();

  wxMenu menu;

  menu.AppendCheckItem(ID_MENU_EXPAND_TAB, kTrExpandTabs);
  menu.AppendSeparator();

  int ts = editor::kMinTabStop;
  int ts_menu_id = ID_MENU_TAB_STOP_0;
  for (; ts <= editor::kMaxTabStop; ++ts, ++ts_menu_id) {
    wxString label = kTrTabStop + wxString::Format(wxT(": %d"), ts);
    menu.AppendCheckItem(ts_menu_id, label);
  }

  menu.AppendSeparator();
  menu.Append(ID_MENU_GUESS_TAB_OPTIONS, kTrGuessTabOptions);

  menu.AppendSeparator();
  menu.Append(ID_MENU_RETAB, kTrRetab);

  if (page_window->expand_tab()) {
    menu.Check(ID_MENU_EXPAND_TAB, true);
  }

  int curr_ts_menu_id = ID_MENU_TAB_STOP_0 + (page_window->tab_stop() - editor::kMinTabStop);
  menu.Check(curr_ts_menu_id, true);

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

// Use sub-menus to simplified the first level menu items.
// Dynamically adjust the first level menu items according to
// the current locale.
void BookFrame::PopupStatusEncodingMenu() {
  wxLocale locale;
  locale.Init();
  int lang = locale.GetLanguage();

  wxMenu menu;

  // UNICODE encodings.
  menu.Append(ID_MENU_ENCODING_UTF8, TR_ENCODING_UTF8);
  menu.Append(ID_MENU_ENCODING_UTF8_BOM, TR_ENCODING_UTF8_BOM);
  menu.Append(ID_MENU_ENCODING_UTF16_BE, TR_ENCODING_UTF16_BE);
  menu.Append(ID_MENU_ENCODING_UTF16_LE, TR_ENCODING_UTF16_LE);

  // Language specific encodings.
  if (lang == wxLANGUAGE_CHINESE_SIMPLIFIED) {
    menu.Append(ID_MENU_ENCODING_GB18030, TR_ENCODING_GB18030);
  } else if (IsTraditionalChinese(lang)) {
    menu.Append(ID_MENU_ENCODING_BIG5, TR_ENCODING_BIG5);
  } else if (lang == wxLANGUAGE_JAPANESE) {
    menu.Append(ID_MENU_ENCODING_SHIFT_JIS, TR_ENCODING_SHIFT_JIS);
    menu.Append(ID_MENU_ENCODING_EUC_JP, TR_ENCODING_EUC_JP);
  }

  wxMenu* sub_menu = new wxMenu;

  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_1, TR_ENCODING_ISO_8859_1);
  sub_menu->Append(ID_MENU_ENCODING_WINDOWS_1250, TR_ENCODING_WINDOWS_1250);
  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_2, TR_ENCODING_ISO_8859_2);

  if (lang != wxLANGUAGE_CHINESE_SIMPLIFIED) {
    sub_menu->Append(ID_MENU_ENCODING_GB18030, TR_ENCODING_GB18030);
  }
  if (!IsTraditionalChinese(lang)) {
    sub_menu->Append(ID_MENU_ENCODING_BIG5, TR_ENCODING_BIG5);
  }

  if (lang != wxLANGUAGE_JAPANESE) {
    sub_menu->Append(ID_MENU_ENCODING_SHIFT_JIS, TR_ENCODING_SHIFT_JIS);
    sub_menu->Append(ID_MENU_ENCODING_EUC_JP, TR_ENCODING_EUC_JP);
  }

  sub_menu->Append(ID_MENU_ENCODING_TIS_620, TR_ENCODING_TIS_620);

  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_5, TR_ENCODING_ISO_8859_5);
  sub_menu->Append(ID_MENU_ENCODING_KOI8_R, TR_ENCODING_KOI8_R);
  sub_menu->Append(ID_MENU_ENCODING_MAC_CYRILLIC, TR_ENCODING_MAC_CYRILLIC);
  sub_menu->Append(ID_MENU_ENCODING_WINDOWS_1251, TR_ENCODING_WINDOWS_1251);

  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_7, TR_ENCODING_ISO_8859_7);
  sub_menu->Append(ID_MENU_ENCODING_WINDOWS_1253, TR_ENCODING_WINDOWS_1253);

  menu.AppendSubMenu(sub_menu, kTrOthers);

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::PopupStatusFileFormatMenu() {
  wxMenu menu;
  menu.AppendCheckItem(ID_MENU_FILE_FORMAT_CRLF, FF_DIAPLAY_NAME_CRLF);
  menu.AppendCheckItem(ID_MENU_FILE_FORMAT_LF, FF_DIAPLAY_NAME_LF);
  menu.AppendCheckItem(ID_MENU_FILE_FORMAT_CR, FF_DIAPLAY_NAME_CR);
  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::PopupStatusFileTypeMenu() {
  wxMenu menu;
  InitFileTypeMenu(&menu, ID_MENU_FILE_TYPE_BEGIN);
  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::OnStatusTabOptionsMenu(wxCommandEvent& evt) {
  if (text_book_->IsEmpty()) {
    return;
  }

  int menu_id = evt.GetId();

  PageWindow* page_window = text_book_->page_window();
  editor::TextBuffer* buffer = page_window->buffer();

  if (menu_id >= ID_MENU_TAB_STOP_0 && menu_id <= ID_MENU_TAB_STOP_8) {
    int tab_stop = editor::kMinTabStop + (menu_id - ID_MENU_TAB_STOP_0);
    buffer->set_tab_stop(tab_stop);
    buffer->Notify(editor::kTabOptionsChange);
    return;
  }

  if (menu_id == ID_MENU_EXPAND_TAB) {
    buffer->set_expand_tab(evt.IsChecked());
    buffer->Notify(editor::kTabOptionsChange);
  } else if (menu_id == ID_MENU_GUESS_TAB_OPTIONS) {
    editor::TabOptions tab_options;
    if (buffer->GuessTabOptions(&tab_options)) {
      buffer->SetTabOptions(tab_options, true);
    } else {
      ShowStatusMessage(kTrCantGuessTabOptions);
    }
  } else if (menu_id == ID_MENU_RETAB) {
    page_window->Retab();
  }
}

void BookFrame::OnStatusEncodingMenu(wxCommandEvent& evt) {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer == NULL) {
    return;
  }

  int index = evt.GetId() - ID_MENU_ENCODING_BEGIN;
  if (index < 0 || index >= editor::ENCODING_COUNT) {
    return;
  }

  editor::EncodingId enc_id = static_cast<editor::EncodingId>(index);
  const editor::Encoding& encoding = editor::GetEncodingById(enc_id);

  buffer->set_file_encoding(encoding);
  buffer->Notify(editor::kEncodingChange);
}

void BookFrame::OnStatusFileFormatMenu(wxCommandEvent& evt) {
  using namespace editor;

  if (text_book_->IsEmpty()) {
    return;
  }

  FileFormat ff = FF_NONE;

  switch (evt.GetId()) {
  case ID_MENU_FILE_FORMAT_CRLF:
    ff = FF_CRLF;
    break;

  case ID_MENU_FILE_FORMAT_LF:
    ff = FF_LF;
    break;

  case ID_MENU_FILE_FORMAT_CR:
    ff = FF_CR;
    break;
  }

  if (ff != FF_NONE) {
    PageWindow* page_window = text_book_->page_window();
    page_window->SetFileFormat(ff);
  }
}

void BookFrame::OnStatusFileTypeMenu(wxCommandEvent& evt) {
  App& app = wxGetApp();

  int index = evt.GetId() - ID_MENU_FILE_TYPE_BEGIN;
  if (index < 0 || index >= app.GetFileTypeCount()) {
    return;
  }

  // Update the file type plugin of the current buffer.

  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer == NULL) {
    return;
  }

  const editor::FileType* ft = app.GetFileType(index);
  if (ft->id != buffer->ft_plugin()->id()) {
    buffer->SetFtPlugin(app.GetFtPlugin(*ft));
  }
}

void BookFrame::ShowStatusMessage(const wxString& msg, int sec) {
  status_bar_->SetMessage(msg, sec * 1000);
}

FindPanel* BookFrame::GetFindPanel() const {
  wxWindow* w = FindWindowById(ID_FIND_PANEL, this);
  if (w == NULL) {
    return NULL;
  } else {
    return wxDynamicCast(w, FindPanel);
  }
}

void BookFrame::ShowFindPanel(int mode) {
  Freeze();

  if (find_panel_ == NULL) {
    find_panel_ = new FindPanel();
    find_panel_->set_session(session_);
    find_panel_->set_mode(mode);
    // Hide to avoid flicker.
    // NOTE: Hide() can be called before the window is created.
    find_panel_->Hide();
    find_panel_->set_theme(theme_->GetTheme(THEME_FIND_PANEL));
    find_panel_->Create(this, ID_FIND_PANEL);

    find_panel_->SetFont(options_->fonts[FONT_FIND_PANEL]);

  } else {
    find_panel_->set_mode(mode);
    find_panel_->UpdateLayout();
  }

  UpdateLayout();
  find_panel_->Show();

  Thaw();

  find_panel_->SetFocus();

  // Find the current selected text.
  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL) {
    PageWindow* page_window = text_book_->page_window();
    if (!page_window->selection().IsEmpty()) {
      editor::TextRange select_range = page_window->selection().range;

      if (select_range.LineCount() == 1) {
        // If there is any text selected in the active page and the selection is
        // inside a single line, it might be what the user wants to find.
        std::wstring find_string;
        page_window->buffer()->GetText(select_range, &find_string);
        find_panel_->SetFindString(wxString(find_string.c_str()));
      }
    }
  }
}

void BookFrame::CloseFindPanel() {
  assert(find_panel_ != NULL);

  Freeze();

  // Destroy find panel.
  find_panel_->Destroy();
  find_panel_ = NULL;

  UpdateLayout();

  Thaw();

  // Transfer focus to text book.
  // TODO: The previous focused might be a tool page.
  text_book_->SetFocus();
}

void BookFrame::OnFindPanelLayoutEvent(wxCommandEvent& evt) {
  UpdateLayout();
}

FindResultPage* BookFrame::GetFindResultPage(bool create) {
  using namespace editor;

  wxWindow* w = FindWindowById(ID_FIND_RESULT_PAGE, tool_book_);

  if (w != NULL) {
    FindResultPage* page = wxDynamicCast(w, FindResultPage);
    assert(page != NULL);
    return page;
  }

  if (!create) {
    return NULL;
  }

  FindResultPage* fr_page = new FindResultPage(fr_buffer_);

  fr_page->set_style(style_);
  fr_page->set_theme(theme_->GetTheme(THEME_TEXT_PAGE));
  fr_page->set_binding(binding_);

#if JIL_ENABLE_LEADER_KEY
  fr_page->set_leader_key(&leader_key_);
#endif

  fr_page->Create(tool_book_->page_area(), ID_FIND_RESULT_PAGE, true);

  fr_page->SetTextFont(options_->fonts[FONT_TEXT]);
  fr_page->SetLinePadding(options_->line_padding);

  Connect(fr_page->GetId(),
          kFindResultPageEvent,
          wxCommandEventHandler(BookFrame::OnFindResultPageEvent));

  // Add but don't activate it.
  tool_book_->AddPage(fr_page, false);

  return fr_page;
}

void BookFrame::ActivateToolPage(BookPage* page) {
  // Show tool book if it's not shown.
  if (!tool_book_->IsShown()) {
    tool_book_->Show();
    splitter_->Split();
  }

  tool_book_->ActivatePage(page, true);
}

BookPage* BookFrame::GetFocusedPage() {
  if (text_book_->HasFocus()) {
    return text_book_->ActivePage();
  }

  if (tool_book_->IsShown() && tool_book_->HasFocus()) {
    return tool_book_->ActivePage();
  }

  return NULL;
}

// TODO: Refine. Maybe don't use GetFocusedPage().
BookPage* BookFrame::GetCurrentPage() {
  BookPage* focused_page = GetFocusedPage();
  if (focused_page != NULL) {
    return focused_page;
  }

  if (text_book_->ActivePage() != NULL) {
    return text_book_->ActivePage();
  }

  if (tool_book_->IsShown() && tool_book_->ActivePage() != NULL) {
    return tool_book_->ActivePage();
  }

  return NULL;
}

//------------------------------------------------------------------------------
// Find & Replace

editor::TextRange BookFrame::Find(TextPage* text_page,
                                  const std::wstring& str,
                                  const editor::TextPoint& point,
                                  int flags,
                                  bool cycle) {
  assert(!str.empty());

  using namespace editor;

  TextBuffer* buffer = text_page->buffer();

  TextRange source_range;

  bool use_regex = GetBit(flags, kFind_UseRegex);
  bool case_sensitive = GetBit(flags, kFind_CaseSensitive);
  bool match_word = GetBit(flags, kFind_MatchWord);
  bool reversely = GetBit(flags, kFind_Reversely);

  if (reversely) {
    source_range.Set(buffer->point_begin(), point);
  } else {
    source_range.Set(point, buffer->point_end());
  }

  TextRange result_range = buffer->FindString(str,
                                              source_range,
                                              use_regex,
                                              case_sensitive,
                                              match_word,
                                              reversely);

  if (result_range.IsEmpty() && cycle) {
    bool re_find = false;

    if (reversely) {
      // Re-find from the end.
      Coord last_line_len = buffer->LineLength(buffer->LineCount());
      if (point.y < buffer->LineCount() || point.x + CoordCast(str.length()) <= last_line_len) {
        source_range.Set(point, buffer->point_end());
        re_find = true;
      }
    } else {
      // Re-find from the begin.
      if (point.y > 1 || point.x >= CoordCast(str.length())) {
        source_range.Set(buffer->point_begin(), point);
        re_find = true;
      }
    }

    if (re_find) {
      result_range = buffer->FindString(str,
                                        source_range,
                                        use_regex,
                                        case_sensitive,
                                        match_word,
                                        reversely);
    }
  }

  return result_range;
}

void BookFrame::FindAll(const std::wstring& str,
                        int flags,
                        editor::TextBuffer* buffer,
                        bool notify,
                        editor::TextBuffer* fr_buffer) {
  std::list<editor::TextRange> result_ranges;
  FindAll(str, flags, buffer, &result_ranges);

  if (result_ranges.empty()) {
    return;
  }

  fr_buffer->FreezeNotify();
  editor::Coord first_ln = fr_buffer->LineCount() + 1;

  AddFrFilePathLine(buffer, fr_buffer);

  AddFrMatchLines(buffer, result_ranges, fr_buffer);

  AddFrMatchCountLine(buffer, result_ranges.size(), fr_buffer);

  fr_buffer->AppendLine(L"");

  fr_buffer->ThawNotify();

  if (notify) {
    editor::Coord last_ln = fr_buffer->LineCount();
    fr_buffer->Notify(editor::kLineAdded, editor::LineRange(first_ln, last_ln));
  }
}

void BookFrame::FindAll(const std::wstring& str,
                        int flags,
                        editor::TextBuffer* buffer,
                        std::list<editor::TextRange>* result_ranges) {
  bool use_regex = GetBit(flags, kFind_UseRegex);
  bool case_sensitive = GetBit(flags, kFind_CaseSensitive);
  bool match_word = GetBit(flags, kFind_MatchWord);

  buffer->FindStringAll(str, buffer->range(), use_regex, case_sensitive, match_word, result_ranges);
}

void BookFrame::ReplaceAll(const std::wstring& str,
                           const std::wstring& replace_str,
                           int flags,
                           TextPage* text_page) {
  using namespace editor;

  TextBuffer* buffer = text_page->buffer();

  std::list<TextRange> result_ranges;
  FindAll(str, flags, buffer, &result_ranges);

  if (result_ranges.empty()) {
    return;
  }

  // If there are multiple result ranges, group the replace actions so
  // that they could be undone together.
  bool group = result_ranges.size() > 1;
  if (group) {
    buffer->AddGroupAction();
  }

  // Replace in reverse order so that one replace action won't break the
  // remaining result ranges.
  std::list<TextRange>::reverse_iterator it = result_ranges.rbegin();
  for (; it != result_ranges.rend(); ++it) {
    // NOTE: Can't group here, because group doesn't support embedding.
    text_page->Replace(*it, replace_str, false);
  }

  if (group) {
    buffer->AddGroupAction();
  }
}

void BookFrame::AddFrFilePathLine(editor::TextBuffer* buffer, editor::TextBuffer* fr_buffer) {
  std::wstring fr_line_data = L"-- ";

  wxString file_path = buffer->file_path_name();
  if (file_path.IsEmpty()) {
    fr_line_data += wxString(kTrPageUntitled).ToStdWstring();
  } else {
    fr_line_data += file_path.ToStdWstring();
  }

  editor::TextLine* fr_line = fr_buffer->AppendLine(fr_line_data);
  fr_line->AddLexElem(0, fr_line->Length(), editor::Lex(editor::kLexComment));
}

void BookFrame::AddFrMatchLines(editor::TextBuffer* buffer,
                                std::list<editor::TextRange>& result_ranges,
                                editor::TextBuffer* fr_buffer) {
  using namespace editor;

  wxString file_path = buffer->file_path_name();

  // Get max line number's string size.
  Coord max_ln = result_ranges.back().point_end().y;
  size_t max_ln_size = base::LexicalCast<std::string>(max_ln).size();

  Lex nr_lex(kLexConstant, kLexConstantNumber);
  Lex id_lex(kLexIdentifier);

  for (const TextRange& range : result_ranges) {
    Coord ln = range.point_begin().y;
    std::wstring ln_str = base::LexicalCast<std::wstring>(ln);

    if (ln_str.size() < max_ln_size) {
      // Right align the line number.
      ln_str.insert(ln_str.begin(), max_ln_size - ln_str.size(), kSpaceChar);
    }

    TextLine* line = buffer->Line(ln);

    std::wstring fr_line_data = ln_str + L" " + line->data();
    TextLine* fr_line = fr_buffer->AppendLine(fr_line_data);

    // Save the file path, buffer id and source line id in the extra data.
    // TODO:
    // If the buffer is temporarily opened, can't save line id, should save line number instead.
    // Can't save buffer id either.
    // Even if the buffer is not temp, it might be closed.
    FrExtraData fr_extra_data = { file_path, buffer->id(), line->id() };
    fr_line->set_extra_data(fr_extra_data);

    // Add lex for line number.
    fr_line->AddLexElem(0, max_ln_size, nr_lex);

    // Add lex for matched sub-string.
    // TODO: Multi-line match when using regex.
    size_t off = range.point_begin().x + max_ln_size + 1;
    size_t len = range.point_end().x - range.point_begin().x;
    fr_line->AddLexElem(off, len, id_lex);
  }
}

void BookFrame::AddFrMatchCountLine(editor::TextBuffer* buffer,
                                    size_t count,
                                    editor::TextBuffer* fr_buffer) {
  std::wstring match_count = L">> ";
  match_count += base::LexicalCast<std::wstring>(count);

  editor::TextLine* fr_line = fr_buffer->AppendLine(match_count);
  fr_line->AddLexElem(0, fr_line->Length(), editor::Lex(editor::kLexComment));
}

void BookFrame::SetFindResult(PageWindow* page_window,
                              const editor::TextRange& find_result,
                              bool incremental) {
  page_window->SetFindResult(find_result, incremental);

  if (find_result.IsEmpty()) {
    return;
  }

  if (incremental) {
    page_window->ScrollToPoint(find_result.point_begin());
  } else {
    page_window->SetSelection(find_result, editor::kForward, false);
    page_window->UpdateCaretPoint(find_result.point_end(), false, true, false);
  }
}

void BookFrame::ClearFindResult(FindResultPage* fr_page) {
  editor::TextBuffer* fr_buffer = fr_page->buffer();
  fr_buffer->ClearText(true);
  //fr_buffer->Line(1)->set_id(editor::kNpos);

  fr_page->UpdateCaretPoint(kPointBegin, false, true, false);
  ActivateToolPage(fr_page);
}

void BookFrame::OnFindThreadEvent(wxThreadEvent& evt) {
  int new_fr_lines = evt.GetInt();

  if (new_fr_lines == wxNOT_FOUND) {
    // Find thread is completed, clear the status message.
    ShowStatusMessage(wxEmptyString);
    return;
  }

  // Notify find result page to refresh.
  if (new_fr_lines > 0) {
    editor::Coord last_ln = fr_buffer_->LineCount();
    if (last_ln > new_fr_lines) {
      editor::Coord first_ln = last_ln - new_fr_lines;
      fr_buffer_->Notify(editor::kLineAdded, editor::LineRange(first_ln, last_ln));
    }
  }

  // Show status message.
  wxString file = evt.GetString();
  wxString msg = wxString::Format(kTrSearchInFile, file);
  ShowStatusMessage(msg);
}

void BookFrame::StopFindThread() {
  {
    wxCriticalSectionLocker locker(find_thread_cs_);
    if (find_thread_ != NULL) {  // Does the thread still exist?
      if (find_thread_->Delete() != wxTHREAD_NO_ERROR) {
        wxLogError("Can't delete find thread!");
      }
      wxLogDebug("Find thread is deleted.");
    }
  }

  // Exit from the critical section to give the thread the possibility to
  // enter its destructor (which is guarded with find_thread_cs_).
  while (true) {
    {
      // Was the ~FindThread() executed?
      wxCriticalSectionLocker locker(find_thread_cs_);
      if (find_thread_ == NULL) {
        break;
      }
    }
    // Wait for thread completion.
    wxMilliSleep(1);
  }
}

void BookFrame::CreateFrBuffer() {
  editor::FileType fr_ft(kFindResultFtId, wxEmptyString);
  editor::FtPlugin* fr_ft_plugin = wxGetApp().GetFtPlugin(fr_ft);

  fr_buffer_ = new editor::TextBuffer(0, fr_ft_plugin, options_->file_encoding);
  fr_buffer_->set_file_name_object(wxFileName::FileName(kTrPageFindResult + wxT(".txt")));

  // Don't scan lex! We set the lex elements manually.
  fr_buffer_->set_scan_lex(false);
}

void BookFrame::DeleteFrBuffer() {
  if (fr_buffer_ != NULL) {
    wxDELETE(fr_buffer_);
  }
}

//------------------------------------------------------------------------------
// Menu

wxMenuItem* BookFrame::AppendMenuItem(wxMenu* menu, int id, const wxString& label, wxItemKind kind) {
  wxString _label = label;

  // Append accelerator.
  editor::Key key = binding_->GetKeyByMenu(id);
  if (!key.IsEmpty()) {
    _label += wxT("\t") + key.ToString();
  }

  wxMenuItem* item = new wxMenuItem(menu, id, _label, wxEmptyString, kind);
  menu->Append(item);

  return item;
}

void BookFrame::LoadMenus() {
  wxMenuBar* menu_bar = new wxMenuBar;

  //------------------------------------
  // OSX Apple menu

#if defined (__WXOSX__)
  // NOTE: The position to insert menu items here is a little confusing.
  // It seems that wxWidgets (or OSX) does something in the background.
  // Anyway, the final menu items are ordered as:
  //   About Jil Text
  //   ----
  //   Preferences...
  //   Theme >

  wxMenu* osx_apple_menu = menu_bar->OSXGetAppleMenu();

  // About
  // NOTE: wxWidgets (or OSX) will display the label correctly as "About Jil Text".
  osx_apple_menu->Insert(0, wxID_ABOUT, wxEmptyString, wxEmptyString, false);

  // Preferences
  // NOTE: wxWidgets (or OSX) will display the label correctly as "Preferences...".
  osx_apple_menu->Insert(0, wxID_PREFERENCES, wxEmptyString, wxEmptyString, false);

  // Theme
  wxMenu* theme_menu = new wxMenu;
  InitThemeMenu(theme_menu);
  osx_apple_menu->Insert(5, wxID_ANY, kTrPrefsTheme, theme_menu);

#endif  // defined (__WXOSX__)

  //------------------------------------
  // File

  wxMenu* file_menu = new wxMenu;
  // - New
  AppendMenuItem(file_menu, ID_MENU_FILE_NEW, kTrFileNew);

#if JIL_MULTIPLE_WINDOW
  file_menu->AppendSeparator();
  AppendMenuItem(file_menu, ID_MENU_FILE_NEW_WINDOW, kTrFileNewFrame);
#endif  // JIL_MULTIPLE_WINDOW

  // - Open
  file_menu->AppendSeparator();
  AppendMenuItem(file_menu, ID_MENU_FILE_OPEN, kTrFileOpen);
  file_menu->AppendSeparator();
  // - Close
  AppendMenuItem(file_menu, ID_MENU_FILE_CLOSE, kTrFileClose);
  AppendMenuItem(file_menu, ID_MENU_FILE_CLOSE_ALL, kTrFileCloseAll);
  file_menu->AppendSeparator();
  AppendMenuItem(file_menu, ID_MENU_FILE_SAVE, kTrFileSave);
  AppendMenuItem(file_menu, ID_MENU_FILE_SAVE_AS, kTrFileSaveAs);
  AppendMenuItem(file_menu, ID_MENU_FILE_SAVE_ALL, kTrFileSaveAll);
  file_menu->AppendSeparator();
  recent_files_menu_ = new wxMenu;
  file_menu->AppendSubMenu(recent_files_menu_, kTrFileRecentFiles);
  UpdateRecentFilesMenu();

  // OSX has quit menu item by default in the apple menu.
#if !defined (__WXOSX__)
  file_menu->AppendSeparator();
  // - Exit
  AppendMenuItem(file_menu, wxID_EXIT, kTrFileExit);
#endif  // !defined (__WXOSX__)

  menu_bar->Append(file_menu, kTrMenuFile);

  //------------------------------------
  // Edit
  // Not loaded here. See RecreateEditMenu.

  edit_menu_ = new wxMenu;
  menu_bar->Append(edit_menu_, kTrMenuEdit);

  //------------------------------------
  // View

  wxMenu* view_menu = new wxMenu;

  AppendMenuItem(view_menu, ID_MENU_VIEW_WRAP, kTrViewWrap, wxITEM_CHECK);
  AppendMenuItem(view_menu, ID_MENU_VIEW_SHOW_NUMBER, kTrViewShowNumber, wxITEM_CHECK);
  AppendMenuItem(view_menu, ID_MENU_VIEW_SHOW_SPACE, kTrViewShowSpace, wxITEM_CHECK);

  view_menu->AppendSeparator();

  AppendMenuItem(view_menu, ID_MENU_VIEW_FULL_SCREEN, kTrViewFullScreen, wxITEM_CHECK);

  menu_bar->Append(view_menu, kTrMenuView);

  //------------------------------------
  // Preferences

  wxMenu* prefs_menu = new wxMenu;

#if !defined (__WXOSX__)

  AppendMenuItem(prefs_menu, wxID_PREFERENCES, kTrPrefsGlobal);

  wxMenu* editor_menu = new wxMenu;
  prefs_menu->AppendSubMenu(editor_menu, kTrPrefsEditor);
  InitFileTypeMenu(editor_menu, ID_MENU_PREFS_EDITOR_BEGIN);

  prefs_menu->AppendSeparator();

  // Theme
  wxMenu* theme_menu = new wxMenu;
  prefs_menu->AppendSubMenu(theme_menu, kTrPrefsTheme);
  InitThemeMenu(theme_menu);

#endif  // !defined (__WXOSX__)

  menu_bar->Append(prefs_menu, kTrMenuPrefs);

  //------------------------------------
  // Help

  wxMenu* help_menu = new wxMenu;
  help_menu->Append(ID_MENU_HELP_VIEW_ONLINE, kTrHelpViewOnline);

#if !defined (__WXOSX__)
  help_menu->Append(wxID_ABOUT, kTrMenuAbout);
#endif

  menu_bar->Append(help_menu, kTrMenuHelp);

  //------------------------------------

  SetMenuBar(menu_bar);
}

void BookFrame::SetAccelForVoidCmds() {
  std::vector<wxAcceleratorEntry> accel_entries;

  size_t void_cmd_count = binding_->GetVoidCmdCount();
  for (size_t i = 0; i < void_cmd_count; ++i) {
    const editor::VoidCmd& void_cmd = binding_->GetVoidCmd(i);

    if (void_cmd.menu != 0) {
      if (!void_cmd.keys.empty()) {
        for (const editor::Key& key : void_cmd.keys) {
          int flags = key.GetAccelFlags();
          accel_entries.push_back(wxAcceleratorEntry(flags, key.code(), void_cmd.menu));
        }
      }
    }
  }

  wxAcceleratorTable accel_table(static_cast<int>(accel_entries.size()), &accel_entries[0]);
  SetAcceleratorTable(accel_table);
}

bool BookFrame::GetFileMenuState(int menu_id) {
  using namespace editor;

  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    BookPage* page = GetCurrentPage();
    if (page != NULL) {
      return page->Page_FileMenuState(menu_id);
    } else {
      return false;
    }
  }

  BookPage* page = text_book_->ActivePage();

  bool state = true;

  switch (menu_id) {
  case ID_MENU_FILE_CLOSE:
    // Fall through.
  case ID_MENU_FILE_CLOSE_ALL:
    // Fall through.
  case ID_MENU_FILE_SAVE_ALL:
    state = (page != NULL);
    break;

  case ID_MENU_FILE_SAVE:
    state = (page != NULL && page->Page_IsModified());
    break;

  default:
    break;
  }

  return state;
}

// Let the current page determine the state.
bool BookFrame::GetEditMenuState(int menu_id) {
  // Allow to show find panel even when no pages are opened.
  if (menu_id == ID_MENU_EDIT_FIND || menu_id == ID_MENU_EDIT_REPLACE) {
    return true;
  }

  BookPage* page = GetCurrentPage();
  if (page != NULL) {
    return page->Page_EditMenuState(menu_id);
  }
  return false;
}

bool BookFrame::GetViewMenuState(int menu_id, bool* check) {
  if (menu_id == ID_MENU_VIEW_FULL_SCREEN) {
    if (check != NULL) {
      *check = IsFullScreen();
      return true;
    }
  }

  TextPage* text_page = ActiveTextPage();
  PageWindow* page_window = text_book_->page_window();

  if (text_page != NULL && check != NULL) {
    switch (menu_id) {
      case ID_MENU_VIEW_WRAP:
        *check = page_window->view_options().wrap;
        break;

      case ID_MENU_VIEW_SHOW_NUMBER:
        *check = page_window->view_options().show_number;
        break;

      case ID_MENU_VIEW_SHOW_SPACE:
        *check = page_window->view_options().show_space;
        break;
    }
  }

  return text_page != NULL;
}

#if JIL_ENABLE_LEADER_KEY
bool BookFrame::GetMenuEnableState(int menu_id) {
  if (menu_id >= ID_MENU_FILE_BEGIN && menu_id < ID_MENU_FILE_END) {
    return GetFileMenuState(menu_id);
  } else if (menu_id >= ID_MENU_EDIT_BEGIN && menu_id < ID_MENU_EDIT_END) {
    return GetEditMenuState(menu_id);
  } else if (menu_id >= ID_MENU_VIEW_BEGIN && menu_id < ID_MENU_VIEW_END) {
    return GetViewMenuState(menu_id);
  }

  return true;
}
#endif  // JIL_ENABLE_LEADER_KEY

void BookFrame::InitThemeMenu(wxMenu* theme_menu) {
  App& app = wxGetApp();
  int count = wxMin(app.GetThemeCount(), kMaxThemes);
  for (int i = 0; i < count; ++i) {
    AppendMenuItem(theme_menu, ID_MENU_THEME_BEGIN + i, app.GetTheme(i), wxITEM_RADIO);
  }
}

void BookFrame::InitFileTypeMenu(wxMenu* ft_menu, int id_begin) {
  App& app = wxGetApp();
  int count = wxMin(app.GetFileTypeCount(), kMaxFileTypes);
  for (int i = 0; i < count; ++i) {
    AppendMenuItem(ft_menu, id_begin + i, app.GetFileType(i)->name);
  }
}

void BookFrame::UpdateRecentFilesMenu() {
  if (recent_files_menu_ == NULL) {
    return;
  }

  size_t count = recent_files_menu_->GetMenuItemCount();
  size_t expected_count = recent_files_.size();
  if (count > expected_count) {
    for (size_t i = expected_count; i < count; ++i) {
      recent_files_menu_->Destroy(ID_MENU_FILE_RECENT_FILE_0 + i);
    }
  } else if (count < expected_count) {
    for (size_t i = count; i < expected_count; ++i) {
      int id = ID_MENU_FILE_RECENT_FILE_0 + i;
      AppendMenuItem(recent_files_menu_, id, wxT("t"));
    }
  }

  std::list<wxString>::iterator it = recent_files_.begin();
  for (size_t i = 0; i < expected_count; ++i, ++it) {
    int id = ID_MENU_FILE_RECENT_FILE_0 + i;
    // NOTE (2016-06-05): Assert fail if use "%d" instead of "%lu" in Linux.
    wxString label = wxString::Format(wxT("%lu. "), i) + *it;
    recent_files_menu_->SetLabel(id, label);
  }
}

//------------------------------------------------------------------------------
// Text Page

TextPage* BookFrame::TextPageByFileName(const wxFileName& fn_object) const {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    if (fn_object == text_pages[i]->buffer()->file_name_object()) {
      return text_pages[i];
    }
  }
  return NULL;
}

TextPage* BookFrame::TextPageByBufferId(size_t buffer_id) const {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    if (buffer_id == text_pages[i]->buffer()->id()) {
      return text_pages[i];
    }
  }
  return NULL;
}

void BookFrame::RemoveAllTextPages(bool from_destroy, const TextPage* except_page) {
  std::vector<TextPage*> text_pages = text_book_->TextPages();

  // If any buffer is modified, ask for save.

  for (TextPage* text_page : text_pages) {
    if (text_page != except_page && text_page->buffer()->modified()) {
      int code = ConfirmSave(text_page);

      if (code == wxCANCEL) {
        return;
      }

      if (code == wxYES) {
        if (!Save(text_page->buffer(), this)) {
          // Fail or cancel to save.
          return;
        }
      }
    }
  }

  text_book_->RemoveAllPages(from_destroy, except_page);
}

void BookFrame::SwitchStackPage(bool forward) {
  std::vector<TextPage*> text_pages = text_book_->StackTextPages();
  if (text_pages.size() <= 1) {
    return;
  }

  editor::SharedTheme theme = theme_->GetTheme(THEME_NAVIGATION_DIALOG);

  NavigationDialog navigation_dialog(theme);
  navigation_dialog.Create(this, wxID_ANY);
  navigation_dialog.SetTextPages(text_pages, forward);
  navigation_dialog.ShowModal();

  size_t index = navigation_dialog.select_index();
  if (index < text_pages.size()) {
    text_book_->ActivatePage(text_pages[index], true);
  }
}

//------------------------------------------------------------------------------
// File - Open, Save

editor::TextBuffer* BookFrame::CreateBuffer(const wxFileName& fn, size_t id, bool scan_lex) {
  using namespace editor;

  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(fn.GetExt());
  TextBuffer* buffer = new TextBuffer(id, ft_plugin, options_->file_encoding);
  buffer->set_scan_lex(scan_lex);

  if (!buffer->LoadFile(fn, options_->cjk_filters)) {
    delete buffer;
    return NULL;
  }

  return buffer;
}

TextPage* BookFrame::DoOpenFile(const wxString& file_name,
                                bool active,
                                bool silent,
                                bool update_recent_files,
                                bool update_recent_files_menu) {
  wxFileName fn(file_name);

  // Make the path absolute, resolve shortcut, etc.
  int flags = wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_SHORTCUT;
  fn.Normalize(flags);

  return DoOpenFile(fn, active, silent, update_recent_files, update_recent_files_menu);
}

TextPage* BookFrame::DoOpenFile(const wxFileName& fn,
                                bool active,
                                bool silent,
                                bool update_recent_files,
                                bool update_recent_files_menu) {
  using namespace editor;

  // Check if this file has already been opened.
  TextPage* text_page = TextPageByFileName(fn);
  if (text_page != NULL) {
    if (active) {
      text_book_->ActivatePage(text_page, true);
    }
  } else {
    TextBuffer* buffer = CreateBuffer(fn, NewBufferId(), true);
    if (buffer == NULL) {
      if (!silent) {
        // Show error message only when it's not silent.
        wxString msg = wxString::Format(kTrFileOpenFail, fn.GetFullPath().c_str());
        ShowError(msg, this);
      }
      return NULL;
    }

    text_page = text_book_->AddPage(buffer, active);
  }

  if (update_recent_files) {
    AddRecentFile(fn.GetFullPath(), update_recent_files_menu);
  }

  return text_page;
}

void BookFrame::DoSaveBuffer(editor::TextBuffer* buffer) {
  assert(buffer != NULL);

  if (!buffer->modified()) {
    return;
  }

  if (buffer->new_created() || buffer->read_only()) {
    SaveBufferAs(buffer, this);
  } else {
    SaveBuffer(buffer, this);
  }
}

void BookFrame::AddRecentFile(const wxString& recent_file, bool update_menu) {
  std::list<wxString>::iterator it = std::find(recent_files_.begin(), recent_files_.end(), recent_file);
  if (it != recent_files_.end()) {
    // Already exists, move to front.
    recent_files_.erase(it);
  }

  recent_files_.push_front(recent_file);

  while (recent_files_.size() > 10) {
    recent_files_.pop_back();
  }

  if (update_menu) {
    UpdateRecentFilesMenu();
  }
}

void BookFrame::RemoveRecentFile(const wxString& recent_file, bool update_menu) {
  recent_files_.remove(recent_file);

  if (update_menu) {
    UpdateRecentFilesMenu();
  }
}

}  // namespace jil
