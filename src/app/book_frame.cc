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
#include "app/i18n_strings.h"
#include "app/navigation_dialog.h"
#include "app/preferences.h"
#include "app/save.h"
#include "app/session.h"
#include "app/skin.h"
#include "app/splitter.h"
#include "app/status_bar.h"
#include "app/text_page.h"
#include "app/text_book.h"
#include "app/tool_book.h"
#include "app/util.h"

namespace jil {

typedef std::list<editor::TextRange> TextRangeList;

static const editor::TextPoint kPointBegin(0, 1);

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
EVT_MENU_RANGE(ID_MENU_THEME_BEGIN, ID_MENU_THEME_END - 1, BookFrame::OnTheme)

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
EVT_MENU_RANGE(ID_MENU_INDENT_BEGIN, ID_MENU_INDENT_END - 1, BookFrame::OnStatusTabOptionsMenu)
EVT_MENU_RANGE(ID_MENU_ENCODING_BEGIN, ID_MENU_ENCODING_END - 1, BookFrame::OnStatusEncodingMenu)
EVT_MENU_RANGE(ID_MENU_FILE_FORMAT_BEGIN, ID_MENU_FILE_FORMAT_END - 1, BookFrame::OnStatusFileFormatMenu)
EVT_MENU_RANGE(ID_MENU_FILE_TYPE_BEGIN, ID_MENU_FILE_TYPE_END - 1, BookFrame::OnStatusFileTypeMenu)

EVT_FIND_PANEL(ID_FIND_PANEL, BookFrame::OnFindPanelEvent)

END_EVENT_TABLE()

BookFrame::BookFrame(Options* options, editor::Options* editor_options, Session* session)
    : options_(options)
    , editor_options_(editor_options)
    , session_(session)
    , splitter_(NULL)
    , tool_book_(NULL)
    , status_bar_(NULL)
    , style_(NULL)
    , find_panel_(NULL)
    , binding_(NULL)
    , recent_files_menu_(NULL) {
  recent_files_ = session_->recent_files();
}

bool BookFrame::Create(wxWindow* parent, wxWindowID id, const wxString& title) {
  if (!wxFrame::Create(parent, id, title)) {
    return false;
  }

  assert(options_ != NULL);
  assert(session_ != NULL);
  assert(theme_.get() != NULL);
  assert(style_ != NULL);
  assert(binding_ != NULL);

  editor::SharedTheme bf_theme = theme_->GetTheme(THEME_BOOK_FRAME);

  splitter_ = new Splitter;
  splitter_->Create(this);
  splitter_->SetBackgroundColour(bf_theme->GetColor(BookFrame::BG));

  // Create text book.
  text_book_ = new TextBook(theme_->GetTheme(THEME_TEXT_BOOK));
  text_book_->Create(splitter_, wxID_ANY);
  text_book_->SetTabFont(options_->fonts[FONT_TABS]);

  Connect(text_book_->GetId(), kEvtBookPageChange, wxCommandEventHandler(BookFrame::OnTextBookPageChange));
  Connect(text_book_->GetId(), kEvtBookPageSwitch, wxCommandEventHandler(BookFrame::OnTextBookPageSwitch));

  // Create tool book.
  tool_book_ = new ToolBook(theme_->GetTheme(THEME_TEXT_BOOK));
  tool_book_->Create(splitter_, wxID_ANY);
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

  // Create status line.
  status_bar_ = new StatusBar;
  status_bar_->set_theme(theme_->GetTheme(THEME_STATUS_BAR));
  status_bar_->SetFields(wxGetApp().status_fields());

  status_bar_->Create(this, ID_STATUS_BAR);

  status_bar_->SetFont(options_->fonts[FONT_STATUS_BAR]);

  status_bar_->SetFieldValue(StatusBar::kField_Cwd, wxGetCwd(), false);

  LoadMenus();
  SetAccelForVoidCmds();

  SetDropTarget(new FileDropTarget(this));

  return true;
}

BookFrame::~BookFrame() {
}

bool BookFrame::Show(bool show) {
  // Restore last state.
  wxRect rect = session_->book_frame_rect();
  if (rect.IsEmpty()) {
    wxClientDisplayRect(&rect.x, &rect.y, &rect.width, &rect.height);
    rect.Deflate(rect.width * 0.125, rect.height * 0.125);
  }
  SetSize(rect);

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

  text_book_->StartBatch();

  // Activate the first opened file.
  bool active = true;
  for (size_t i = 0; i < file_names.size(); ++i) {
    // Don't update Recent Files menu for performance. Update it later.
    if (DoOpenFile(file_names[i], active, silent, true, false) != NULL) {
      active = false;
    }
  }

  text_book_->EndBatch();

  UpdateRecentFilesMenu();
}

// Helper class for restoring last opened files.
class OpenedPage {
public:
  BookPage* page;
  int stack_index;
};

bool operator<(const OpenedPage& lhs, const OpenedPage& rhs) {
  return lhs.stack_index > rhs.stack_index;
}

// TODO: Do this in another thread. Don't block the UI.
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

    text_book_->ActivatePage(last_page.page);
  }

  text_book_->EndBatch();

  UpdateRecentFilesMenu();
}

void BookFrame::FileNew() {
  using namespace editor;

  // TODO: Let the user choose file type.
  FileType txt_ft(kTxtFtId, kTrPlainText);

  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(txt_ft);
  TextBuffer* buffer = TextBuffer::Create(NewBufferId(), ft_plugin, options_->file_encoding);

  TextPage* text_page = CreateTextPage(buffer, text_book_->PageParent(), wxID_ANY);
  text_book_->AddPage(text_page, true);
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
  TextPage* text_page = text_book_->ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  // If the buffer is modified, ask for save.
  if (text_page->buffer_modified()) {
    int code = ConfirmSave(text_page);

    if (code == wxCANCEL) {
      return;  // Don't close.
    }

    if (code == wxYES) {
      if (!Save(text_page->buffer(), this)) {
        // Fail or cancel to save. Don't close.
        return;
      }
    }
  }

  text_book_->RemovePage(text_page);
}

void BookFrame::FileCloseAll() {
  RemoveAllPages();
}

void BookFrame::FileCloseAllButThis() {
  RemoveAllPages(text_book_->ActiveTextPage());
}

void BookFrame::FileSave() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer != NULL) {
    DoSaveBuffer(buffer);
  }
}

void BookFrame::FileSaveAs() {
  // NOTE: Save As applies to not only text page, but also tool pages, e.g.,
  // find result page.
  BookPage* page = GetCurrentPage();
  if (page != NULL) {
    page->Page_OnSaveAs();
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
  TextPage* text_page = text_book_->ActiveTextPage();
  if (text_page != NULL) {
    if (wxTheClipboard->Open()) {
      wxTheClipboard->SetData(new wxTextDataObject(text_page->buffer()->file_path_name()));
      wxTheClipboard->Close();
    }
  }
}

void BookFrame::FileOpenFolder() {
  TextPage* text_page = text_book_->ActiveTextPage();
  if (text_page != NULL) {
    ExploreFile(text_page->buffer()->file_path_name());
  }
}

//------------------------------------------------------------------------------

size_t BookFrame::PageCount() const {
  return text_book_->PageCount();
}

void BookFrame::SwitchToNextPage() {
  text_book_->SwitchToNextPage();
}

void BookFrame::SwitchToPrevPage() {
  text_book_->SwitchToPrevPage();
}

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

void BookFrame::Wrap() {
  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL) {
    bool wrap = !text_page->view_options().wrap;
    text_page->Wrap(wrap);
  }
}

void BookFrame::ShowNumber() {
  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL) {
    bool show_number = !text_page->view_options().show_number;
    text_page->ShowNumber(show_number);
  }
}

void BookFrame::ShowSpace() {
  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL) {
    bool show_space = !text_page->view_options().show_space;
    text_page->ShowSpace(show_space);
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
    last_screen_rect_ = GetScreenRect();
  }

  ShowFullScreen(show, style);
}

TextPage* BookFrame::ActiveTextPage() const {
  TextPage* text_page = text_book_->ActiveTextPage();
  if (text_page == NULL) {
    return NULL;
  }

  if (text_page->IsBeingDeleted()) {
    return NULL;
  }

  return text_page;
}

editor::TextBuffer* BookFrame::ActiveBuffer() const {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return NULL;
  }
  return text_page->buffer();
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
  if (i < recent_files_.size()) {
    std::list<wxString>::iterator it = recent_files_.begin();
    std::advance(it, i);
    // NOTE: Don't have to update recent files.
    DoOpenFile(*it, true, false, false, false);
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

  PrefGlobalDialog dialog(options_);
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

  if (options_->line_padding != old_options.line_padding) {
    ApplyLinePadding(options_->line_padding);
  }

  if (options_->fonts[FONT_TEXT] != old_options.fonts[FONT_TEXT]) {
    ApplyTextFont(options_->fonts[FONT_TEXT]);
  }

  if (options_->fonts[FONT_LINE_NR] != old_options.fonts[FONT_LINE_NR]) {
    ApplyLineNrFont(options_->fonts[FONT_LINE_NR]);
  }

  if (options_->fonts[FONT_TABS] != old_options.fonts[FONT_TABS]) {
    text_book_->SetTabFont(options_->fonts[FONT_TABS]);
    tool_book_->SetTabFont(options_->fonts[FONT_TABS]);
  }

  if (options_->fonts[FONT_STATUS_BAR] != old_options.fonts[FONT_STATUS_BAR]) {
    status_bar_->SetFont(options_->fonts[FONT_STATUS_BAR]);
    UpdateLayout();
  }
}

void BookFrame::ApplyLinePadding(int line_padding) {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (TextPage* text_page : text_pages) {
    text_page->SetLinePadding(line_padding);
  }

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page != NULL) {
    fr_page->SetLinePadding(line_padding);
  }
}

void BookFrame::ApplyTextFont(const wxFont& font) {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (TextPage* text_page : text_pages) {
    text_page->SetTextFont(font);
  }

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page != NULL) {
    fr_page->SetTextFont(font);
  }
}

void BookFrame::ApplyLineNrFont(const wxFont& font) {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (TextPage* text_page : text_pages) {
    text_page->SetLineNrFont(font);
  }

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page != NULL) {
    fr_page->SetLineNrFont(font);
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

  // Backup the old optinos.
  editor::Options old_options = ft_plugin->options();

  // This will be the new options.
  editor::Options options = ft_plugin->options();

  wxString title = kTrOptions + wxT(" - ") + ft->name;
  PrefEditorDialog dialog(&options);
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
    if (text_page->ft_plugin()->id() != ft_id) {
      continue;
    }

    editor::TextBuffer* buffer = text_page->buffer();

    // Apply text option changes.

    buffer->set_text_options(options.text);

    if (options.text.tab_stop != old_options.text.tab_stop ||
        options.text.expand_tab != old_options.text.expand_tab) {
      // Notify to update the copy in text window and the status field.
      buffer->Notify(editor::kTabOptionsChange);
    }

    // Apply view option changes.

    if (view_options_changed) {
      buffer->set_view_options(options.view);

      text_page->Freeze();

      if (options.view.wrap != old_options.view.wrap) {
        text_page->Wrap(options.view.wrap);
      }

      if (options.view.show_number != old_options.view.show_number) {
        text_page->ShowNumber(options.view.show_number);
      }

      if (options.view.show_space != old_options.view.show_space) {
        text_page->ShowSpace(options.view.show_space);
      }

      if (options.view.show_hscrollbar != old_options.view.show_hscrollbar) {
        text_page->ShowHScrollbar(options.view.show_hscrollbar);
      }

      if (options.view.rulers != old_options.view.rulers) {
        text_page->SetRulers(options.view.rulers);
      }

      text_page->Thaw();
    }
  }
}

void BookFrame::OnTheme(wxCommandEvent& evt) {
  App& app = wxGetApp();

  int index = evt.GetId() - ID_MENU_THEME_BEGIN;
  wxString theme_name = app.GetTheme(index);
  if (theme_name == options_->theme) {
    return;
  }

  if (!app.ReloadTheme(theme_name)) {
    return;
  }

  options_->theme = theme_name;
  app.SaveUserGlobalOptions();

  // Apply theme.

  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (TextPage* text_page : text_pages) {
    text_page->ReapplyTheme();
  }
  TextPage* active_text_page = text_book_->ActiveTextPage();
  if (active_text_page != NULL) {
    active_text_page->Refresh();
  }

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
  // Delegate to the focused page to handle it.
  BookPage* page = GetFocusedPage();
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
  int menu_id = evt.GetId();
  wxString label;
  bool state = GetFileMenuState(menu_id, &label);
  evt.Enable(state);
  if (!label.IsEmpty()) {
    evt.SetText(label);
  }
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

void BookFrame::OnClose(wxCloseEvent& evt) {
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

  RemoveAllPages();

  if (PageCount() > 0) {
    return;
  }

  session_->set_recent_files(recent_files_);

  if (IsMaximized()) {
    session_->set_book_frame_maximized(true);
  } else {
    session_->set_book_frame_maximized(false);
    if (IsFullScreen()) {
      session_->set_book_frame_rect(last_screen_rect_);
    } else {
      session_->set_book_frame_rect(GetScreenRect());
    }
  }

  if (find_panel_ != NULL) {
    session_->set_find_flags(find_panel_->flags());
  }

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
  TextPage* text_page = text_book_->ActiveTextPage();

  if (text_page == NULL) {
    status_bar_->ClearFieldValues();
  } else {
    // Update field values.
    editor::TextBuffer* buffer = text_page->buffer();

    // TODO: Don't update the field if it's not shown.
    if (options_->switch_cwd && !buffer->new_created()) {
      wxString cwd = buffer->file_path(wxPATH_GET_VOLUME, wxPATH_NATIVE);
      wxSetWorkingDirectory(cwd);
      status_bar_->SetFieldValue(StatusBar::kField_Cwd, cwd, false);
    }

    UpdateStatusCaret(text_page, false);
    UpdateStatusTabOptions(text_page, false);
    UpdateStatusEncoding(text_page, false);
    UpdateStatusFileFormat(text_page, false);
    UpdateStatusFileType(text_page, false);
  }

  status_bar_->UpdateFieldSizes();
  status_bar_->Refresh();
}

void BookFrame::UpdateStatusCaret(TextPage* page, bool refresh) {
  status_bar_->SetFieldValue(StatusBar::kField_Caret,
                             GetStatusCaretString(page),
                             refresh);
}

void BookFrame::UpdateStatusTabOptions(TextPage* page, bool refresh) {
  status_bar_->SetFieldValue(StatusBar::kField_TabOptions,
                             GetStatusTabOptionsString(page),
                             refresh);
}

void BookFrame::UpdateStatusEncoding(TextPage* page, bool refresh) {
  status_bar_->SetFieldValue(StatusBar::kField_Encoding,
                             page->buffer()->file_encoding().display_name,
                             refresh);
}

void BookFrame::UpdateStatusFileFormat(TextPage* page, bool refresh) {
  status_bar_->SetFieldValue(StatusBar::kField_FileFormat,
                             GetFileFormatName(page->buffer()->file_format()),
                             refresh);
}

void BookFrame::UpdateStatusFileType(TextPage* page, bool refresh) {
  status_bar_->SetFieldValue(StatusBar::kField_FileType,
                             page->buffer()->ft_plugin()->name(),
                             refresh);
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

wxString BookFrame::GetStatusCaretString(TextPage* page) const {
  wxString format = kTrLine + wxT(" %d, ") + kTrColumn + wxT(" %d");
  return wxString::Format(format, page->caret_point().y, page->caret_point().x);
}

wxString BookFrame::GetStatusTabOptionsString(TextPage* page) const {
  wxString indent_str = page->expand_tab() ? _("Space") : _("Tab");
  indent_str += wxString::Format(wxT(" - %d"), page->tab_stop());
  return indent_str;
}

// Update status bar according to the event.
void BookFrame::OnTextWindowEvent(wxCommandEvent& evt) {
  using namespace editor;

  int type = evt.GetInt();

  if (type == TextWindow::kGetFocusEvent) {
    HandleTextWindowGetFocus(evt);
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

  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  // The text page posting this event is not the active page.
  if (text_page != evt.GetEventObject()) {
    return;
  }

  // Caret event is very frequently, so avoid updating field sizes.
  if (type == TextWindow::kCaretEvent) {
    UpdateStatusCaret(text_page, true);
    return;
  }

  if (type == TextWindow::kEncodingEvent) {
    UpdateStatusEncoding(text_page, false);
  } else if (type == TextWindow::kFileTypeEvent) {
    UpdateStatusFileType(text_page, false);
  } else if (type == TextWindow::kFileFormatEvent) {
    UpdateStatusFileFormat(text_page, false);
  } else if (type == TextWindow::kTabOptionsEvent) {
    UpdateStatusTabOptions(text_page, false);
  }

  status_bar_->UpdateFieldSizes();
  status_bar_->Refresh();
}

// Update menus according to the focused text window.
void BookFrame::HandleTextWindowGetFocus(wxCommandEvent& evt) {
  wxMenuBar* menu_bar = GetMenuBar();
  if (menu_bar == NULL) {
    return;
  }

  wxMenu* edit_menu = menu_bar->GetMenu(1);
  if (edit_menu == NULL) {
    return;
  }

  // NOTE: Don't try to get the focused page from evt.GetEventObject().
  // A window could be destroyed immediately after it gets focus.
  BookPage* focused_page = GetFocusedPage();
  if (focused_page == NULL) {
    return;
  }

  wxString page_type = focused_page->Page_Type();

  if (page_type != page_type_) {
    page_type_ = page_type;
    wxLogDebug("Current page type: %s", page_type_);

    ClearMenuItems(edit_menu);
    focused_page->Page_EditMenu(edit_menu);
  }
}

void BookFrame::OnFindResultPageEvent(wxCommandEvent& evt) {
  using namespace editor;

  FindResultPage* fr_page = GetFindResultPage(false);
  if (fr_page == NULL) {
    return;
  }

  int type = evt.GetInt();

  if (type == FindResultPage::kLocalizeEvent) {
    Coord fr_ln = fr_page->caret_point().y;

    TextBuffer* fr_buffer = fr_page->buffer();
    TextLine* fr_line = fr_buffer->Line(fr_ln);

    const boost::any& any = fr_line->extra_data();
    if (any.empty()) {
      return;
    }

    FrExtraData fr_extra_data;

    try {
      fr_extra_data = boost::any_cast<FrExtraData>(any);
    } catch (boost::bad_any_cast& ) {
      return;
    }

    TextPage* text_page = NULL;

    if (!fr_extra_data.file_path.IsEmpty()) {
      wxFileName fn_object(fr_extra_data.file_path);
      text_page = TextPageByFileName(fn_object);

      if (text_page != NULL) {
        // The page might not be active, activate it.
        text_book_->ActivatePage(text_page);
        text_book_->SetFocus();
      } else {
        // The page is closed? Reopen it.
        text_page = DoOpenFile(fn_object, true, false, NULL);

        if (text_page != NULL) {
          AddRecentFile(fn_object.GetFullPath());
          UpdateRecentFilesMenu();
        } // else: Failed to reopen it.
      }
    } else if (fr_extra_data.buffer_id) {
      // The buffer has no file path, a new buffer.
      text_page = TextPageByBufferId(fr_extra_data.buffer_id);

      if (text_page != NULL) {
        // The page might not be active, activate it.
        text_book_->ActivatePage(text_page);
        text_book_->SetFocus();
      }
    }

    if (text_page == NULL) {
      return;
    }

    // Go to the source line.
    Coord ln = text_page->buffer()->LineNrFromId(fr_extra_data.line_id);
    if (ln != kInvCoord) {
      text_page->Goto(ln);
    }
  }
}

void BookFrame::OnStatusFieldClick(wxCommandEvent& evt) {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer == NULL) {
    return;
  }

  int field_id = evt.GetInt();

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
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  wxMenu menu;

  menu.AppendCheckItem(ID_MENU_EXPAND_TAB, kTrExpandTabs);
  menu.AppendSeparator();

  int ts = editor::kMinTabStop;
  int ts_menu_id = ID_MENU_TAB_STOP_0;
  for (; ts < editor::kMaxTabStop; ++ts, ++ts_menu_id) {
    wxString label = kTrTabStop + wxString::Format(wxT(": %d"), ts);
    menu.AppendCheckItem(ts_menu_id, label);
  }

  menu.AppendSeparator();
  menu.Append(ID_MENU_GUESS_TAB_OPTIONS, kTrGuessTabOptions);

  menu.AppendSeparator();
  menu.Append(ID_MENU_RETAB, kTrRetab);

  if (text_page->expand_tab()) {
    menu.Check(ID_MENU_EXPAND_TAB, true);
  }

  int curr_ts_menu_id = ID_MENU_TAB_STOP_0 + (text_page->tab_stop() - editor::kMinTabStop);
  menu.Check(curr_ts_menu_id, true);

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

static bool IsTraditionalChinese(int lang) {
  return (lang >= wxLANGUAGE_CHINESE_TRADITIONAL &&
          lang <= wxLANGUAGE_CHINESE_TAIWAN);
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
  menu.Append(ID_MENU_FILE_FORMAT_CRLF, FF_DIAPLAY_NAME_CRLF);
  menu.Append(ID_MENU_FILE_FORMAT_LF, FF_DIAPLAY_NAME_LF);
  menu.Append(ID_MENU_FILE_FORMAT_CR, FF_DIAPLAY_NAME_CR);
  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::PopupStatusFileTypeMenu() {
  wxMenu menu;
  InitFileTypeMenu(&menu, ID_MENU_FILE_TYPE_BEGIN);
  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::OnStatusTabOptionsMenu(wxCommandEvent& evt) {
  int menu_id = evt.GetId();

  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  editor::TextBuffer* buffer = text_page->buffer();

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
      // TODO: Show error message on status bar.
    }
  } else if (menu_id == ID_MENU_RETAB) {
    text_page->Retab();
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

  TextPage* active_page = ActiveTextPage();
  if (active_page == NULL) {
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
    active_page->SetFileFormat(ff);
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
    find_panel_ = new FindPanel(session_, mode);
    // Hide to void flicker. (Yes, Hide() can be called before the window is created.)
    find_panel_->Hide();
    find_panel_->set_theme(theme_->GetTheme(THEME_FIND_PANEL));
    find_panel_->Create(this, ID_FIND_PANEL);
    find_panel_->SetLocation(kCurrentPage);  // TODO
  } else {
    find_panel_->set_mode(mode);
    find_panel_->UpdateLayout();
  }

  UpdateLayout();
  find_panel_->Show();

  Thaw();

  find_panel_->SetFocus();

  // Find the selected text.
  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL && !text_page->selection().IsEmpty()) {
    editor::TextRange select_range = text_page->selection().range;

    if (select_range.LineCount() == 1) {
      // If there is any text selected in the active page and the selection is
      // inside a single line, it might be what the user wants to find.
      std::wstring find_string;
      text_page->buffer()->GetText(select_range, &find_string);
      find_panel_->SetFindString(wxString(find_string.c_str()));
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

void BookFrame::OnFindPanelEvent(FindPanelEvent& evt) {
  int event_type = evt.GetInt();

  if (event_type == FindPanel::kLayoutEvent) {
    UpdateLayout();
    return;
  }

  FindLocation location = evt.location();

  switch (event_type) {
    case FindPanel::kFindStrEvent:
      HandleFindStrChange(evt.find_str(), evt.flags());
      break;

    case FindPanel::kFindEvent:
      // Find next matching result.
      if (location == kCurrentPage) {
        FindInActivePage(evt.find_str(), evt.flags());
      } else if (location == kAllPages) {
        FindInAllPages(evt.find_str(), evt.flags());
      }
      break;

    case FindPanel::kFindAllEvent:
      if (location == kCurrentPage) {
        FindAllInActivePage(evt.find_str(), evt.flags());
      } else if (location == kAllPages) {
        FindAllInAllPages(evt.find_str(), evt.flags());
      }
      // Close find panel after find all.
      //CloseFindPanel();
      break;

    case FindPanel::kReplaceEvent:
      ReplaceInActivePage(evt.find_str(), evt.replace_str(), evt.flags());
      break;

    case FindPanel::kReplaceAllEvent:
      ReplaceAllInActivePage(evt.find_str(), evt.replace_str(), evt.flags());
      break;
  }
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

  FileType ft(kFtId_FindResult, wxEmptyString);
  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(ft);

  TextBuffer* buffer = TextBuffer::Create(0, ft_plugin, options_->file_encoding);
  buffer->set_file_name_object(wxFileName::FileName(kTrPageFindResult + wxT(".txt")));

  FindResultPage* fr_page = new FindResultPage(buffer);

  fr_page->set_style(style_);
  fr_page->set_theme(theme_->GetTheme(THEME_TEXT_PAGE));
  fr_page->set_binding(binding_);

#if JIL_ENABLE_LEADER_KEY
  fr_page->set_leader_key(&leader_key_);
#endif

  fr_page->Create(tool_book_->PageParent(), ID_FIND_RESULT_PAGE, true);

  fr_page->SetTextFont(options_->fonts[FONT_TEXT]);
  fr_page->SetLineNrFont(options_->fonts[FONT_LINE_NR]);
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

  tool_book_->ActivatePage(page);
}

BookPage* BookFrame::GetFocusedPage() {
  // Check text book.
  if (text_book_->HasFocus()) {
    return text_book_->ActivePage();
  }

  // Check tool book.
  if (tool_book_->IsShown() && tool_book_->HasFocus()) {
    return tool_book_->ActivePage();
  }

  return NULL;
}

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

void BookFrame::HandleFindStrChange(const std::wstring& str, int flags) {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  if (str.empty()) {
    // Find string changes to empty, clear find result.
    text_page->ClearFindResult();
    return;
  }

  editor::TextPoint point = text_page->caret_point();
  if (!text_page->selection().IsEmpty()) {
    point = text_page->selection().begin();
  }

  editor::TextRange find_result = Find(text_page, str, point, flags, true);
  SetFindResult(text_page, find_result, true);
}

void BookFrame::FindInActivePage(const std::wstring& str, int flags) {
  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  editor::TextPoint point = text_page->caret_point();

  if (GetBit(flags, kFind_Reversely)) {
    // If there's any selected text, it might be the last find result. We don't
    // check it (for simplicity). But the find start point has to be adjusted,
    // otherwise the find result will always be this selected text.
    if (!text_page->selection().IsEmpty()) {
      point = text_page->selection().begin();
    }
  }

  editor::TextRange find_result = Find(text_page, str, point, flags, true);

  if (!find_result.IsEmpty()) {
    if (find_result == text_page->find_result() && !text_page->inc_find()) {
      // The find result is the same as last time.
      // TODO: Display a message on status bar.
    }
  }

  SetFindResult(text_page, find_result, false);
}

void BookFrame::FindInAllPages(const std::wstring& str, int flags) {
  using namespace editor;

  TextPage* text_page = text_book_->ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  // Find in active text page.
  TextRange result_range = Find(text_page, str, text_page->caret_point(), flags, false);

  if (!result_range.IsEmpty()) {
    SelectFindResult(text_page, result_range);
    return;
  }

  TextPage* start_text_page = text_page;

  do {
    // Go to next page.
    text_page = AsTextPage(text_book_->NextPage(text_page));
    if (text_page == NULL) {
      break;
    }

    if (text_page == start_text_page) {
      // Back to the start page.
      result_range = Find(text_page, str, kPointBegin, flags, false);
      break;
    }

    result_range = Find(text_page, str, kPointBegin, flags, false);

  } while (result_range.IsEmpty());

  if (result_range.IsEmpty()) {
    return;  // No match
  }

  if (text_page == start_text_page) {
    return;
  }

  // Activate the text page where the match is found.
  text_book_->ActivatePage(text_page);

  SelectFindResult(text_page, result_range);
}

void BookFrame::FindAllInActivePage(const std::wstring& str, int flags) {
  FindResultPage* fr_page = GetFindResultPage(true);

  // Clear previous result.
  editor::TextRange range = fr_page->buffer()->range();
  if (!range.IsEmpty()) {
    fr_page->buffer()->DeleteText(range);
  }
  fr_page->buffer()->Line(1)->set_id(editor::kNpos);  // Clear line ID

  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL) {
    FindAll(str, text_page->buffer(), flags, fr_page);
  }

  // Reset caret point.
  fr_page->UpdateCaretPoint(kPointBegin, false, true, false);

  ActivateToolPage(fr_page);
}

void BookFrame::FindAllInAllPages(const std::wstring& str, int flags) {
  FindResultPage* fr_page = GetFindResultPage(true);

  // Clear previous result.
  editor::TextRange range = fr_page->buffer()->range();
  if (!range.IsEmpty()) {
    fr_page->buffer()->DeleteText(range);
  }
  fr_page->buffer()->Line(1)->set_id(editor::kNpos);  // Clear line ID

  std::vector<TextPage*> text_pages = text_book_->TextPages();
  for (size_t i = 0; i < text_pages.size(); ++i) {
    FindAll(str, text_pages[i]->buffer(), flags, fr_page);
  }

  // Reset caret point.
  fr_page->UpdateCaretPoint(kPointBegin, false, true, false);

  ActivateToolPage(fr_page);
}

void BookFrame::ReplaceInActivePage(const std::wstring& str,
                                    const std::wstring& replace_str,
                                    int flags) {
  using namespace editor;

  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  // Backup the selection.
  Selection selection = text_page->selection();

  TextPoint point = text_page->caret_point();

  // If there's any selected text, the find start point has to be adjusted to
  // the begin point of the selection.
  if (!selection.IsEmpty()) {
    point = selection.begin();
  }

  TextRange result_range = Find(text_page, str, point, flags, true);
  if (result_range.IsEmpty()) {
    return;
  }

  // If the find result is not the current selection, select it.
  // TODO: Rect selection.
  if (result_range != selection.range) {
    SelectFindResult(text_page, result_range);
    return;
  }

  // The find result is the current selection, replace it.
  text_page->ClearSelection();

  bool grouped = !replace_str.empty();
  text_page->DeleteSelection(grouped, false);
  if (!replace_str.empty()) {
    text_page->InsertString(result_range.point_begin(), replace_str, grouped, false);
  }

  // Go to next match.
  point = text_page->caret_point();
  result_range = Find(text_page, str, point, flags, true);
  if (!result_range.IsEmpty()) {
    SelectFindResult(text_page, result_range);
  }
}

void BookFrame::ReplaceAllInActivePage(const std::wstring& str,
                                       const std::wstring& replace_str,
                                       int flags) {
  using namespace editor;

  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  TextBuffer* buffer = text_page->buffer();

  bool use_regex = GetBit(flags, kFind_UseRegex);
  bool case_sensitive = GetBit(flags, kFind_CaseSensitive);
  bool match_word = GetBit(flags, kFind_MatchWord);

  TextRange source_range = buffer->range();
  TextRange result_range;

  size_t count = 0;

  while (true) {
    result_range = buffer->FindString(str,
                                      source_range,
                                      use_regex,
                                      case_sensitive,
                                      match_word,
                                      false);

    if (result_range.IsEmpty()) {
      break;
    }

    if (count == 0) {
      text_page->Exec(new GroupAction(text_page->buffer()));
    }

    ++count;

    text_page->DeleteRange(result_range, kForward, false, false, false, false);
    if (!replace_str.empty()) {
      text_page->InsertString(result_range.point_begin(), replace_str, false, false);
    }

    // Update source range and find next.
    TextPoint point_begin = result_range.point_begin();
    point_begin.x += CoordCast(replace_str.length());
    source_range.Set(point_begin, source_range.point_end());
  }

  if (count > 0) {
    text_page->Exec(new GroupAction(text_page->buffer()));
  }
}

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
  // Reversely regex find is not supported.
  bool reversely = !use_regex && GetBit(flags, kFind_Reversely);

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
    bool refind = false;

    if (reversely) {
      // Re-find from the end.
      Coord last_line_len = buffer->LineLength(buffer->LineCount());
      if (point.y < buffer->LineCount() || point.x + CoordCast(str.length()) <= last_line_len) {
        source_range.Set(point, buffer->point_end());
        refind = true;
      }
    } else {
      // Re-find from the begin.
      if (point.y > 1 || point.x >= CoordCast(str.length())) {
        source_range.Set(buffer->point_begin(), point);
        refind = true;
      }
    }

    if (refind) {
      result_range = buffer->FindString(str, source_range, use_regex,
        case_sensitive, match_word, reversely);
    }
  }

  return result_range;
}

void BookFrame::FindAll(const std::wstring& str,
                        editor::TextBuffer* buffer,
                        int flags,
                        FindResultPage* fr_page) {
  using namespace editor;

  std::list<TextRange> result_ranges;
  buffer->FindStringAll(str,
                        buffer->range(),
                        GetBit(flags, kFind_UseRegex),
                        GetBit(flags, kFind_CaseSensitive),
                        GetBit(flags, kFind_MatchWord),
                        &result_ranges);

  if (result_ranges.empty()) {
    return;
  }

  // Add file path name line.
  wxString file_path = buffer->file_path_name();

  std::wstring fr_line_data = L"-- ";
  if (file_path.IsEmpty()) {
    fr_line_data += wxString(kTrPageUntitled).ToStdWstring();
  } else {
    fr_line_data += file_path.ToStdWstring();
  }
  fr_page->buffer()->AppendLine(fr_line_data);

  // Get max line number's string size.
  Coord max_ln = result_ranges.back().point_end().y;
  size_t max_ln_size = base::LexicalCast<std::string>(max_ln).size();

  std::list<TextRange>::iterator range_it = result_ranges.begin();
  for (; range_it != result_ranges.end(); ++range_it) {
    const TextRange& range = *range_it;

    Coord ln = range.point_begin().y;
    std::wstring ln_str = base::LexicalCast<std::wstring>(ln);
    if (ln_str.size() < max_ln_size) {
      // Right align the line number.
      ln_str.insert(ln_str.begin(), max_ln_size - ln_str.size(), kSpaceChar);
    }

    TextLine* line = buffer->Line(ln);

    std::wstring fr_line_data = ln_str + L" " + line->data();
    TextLine* fr_line = fr_page->buffer()->AppendLine(fr_line_data);

    // Save the file path, buffer id and source line id in the extra data.
    FrExtraData fr_extra_data = { file_path, buffer->id(), line->id() };
    fr_line->set_extra_data(fr_extra_data);

    // Add lex element for the prefix line number.
    fr_line->AddLexElem(0, max_ln_size, Lex(kLexConstant, kLexConstantNumber));

    // Add lex element for the matched string.
    // TODO: Multiple line match when using regex.
    size_t off = range.point_begin().x + max_ln_size + 1;
    size_t len = range.point_end().x - range.point_begin().x;
    fr_line->AddLexElem(off, len, Lex(kLexIdentifier));
  }

  // Add match count line.
  // Example: >> 4
  std::wstring match_count = L">> ";
  match_count += base::LexicalCast<std::wstring>(result_ranges.size());
  fr_page->buffer()->AppendLine(match_count);
  fr_page->buffer()->AppendLine(L"");
}
 
void BookFrame::SelectFindResult(TextPage* text_page, const editor::TextRange& result_range) {
  text_page->SetSelection(result_range, editor::kForward, false);

  // Don't scroll, Goto will scroll.
  text_page->UpdateCaretPoint(result_range.point_end(), false, false, false);

  // Always scroll to the begin point of the result range. This makes more
  // sense especially for multiple-line result range (regex find only).
  // Don't use ScrollToPoint().
  // TODO: Horizontally scroll if necessary.
  text_page->Goto(result_range.line_first());
}

void BookFrame::SetFindResult(TextPage* text_page,
                              const editor::TextRange& find_result,
                              bool incremental) {
  text_page->SetFindResult(find_result, incremental);

  if (find_result.IsEmpty()) {
    return;
  }

  if (incremental) {
    text_page->ScrollToPoint(find_result.point_begin());
  } else {
    text_page->SetSelection(find_result, editor::kForward, false);
    text_page->UpdateCaretPoint(find_result.point_end(), false, true, false);
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
  osx_apple_menu->Insert(5, wxID_ANY, kTrToolsTheme, theme_menu);

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

  // Edit menu items will be created when some page is focused.
  // See: virtual bool BookPage::Page_OnMenu(int menu_id) = 0;

  wxMenu* edit_menu = new wxMenu;
  menu_bar->Append(edit_menu, kTrMenuEdit);

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
  prefs_menu->AppendSubMenu(theme_menu, kTrToolsTheme);
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

bool BookFrame::GetFileMenuState(int menu_id, wxString* text) {
  using namespace editor;

  if (menu_id == ID_MENU_FILE_SAVE_AS) {
    BookPage* page = GetCurrentPage();
    if (page != NULL) {
      return page->Page_FileMenuState(menu_id, text);
    } else {
      if (text != NULL) {
        *text = kTrFileSaveAs;
      }
      return false;
    }
  }

  TextPage* text_page = text_book_->ActiveTextPage();

  bool state = true;

  switch (menu_id) {
  case ID_MENU_FILE_CLOSE:
    // Fall through.
  case ID_MENU_FILE_CLOSE_ALL:
    // Fall through.
  case ID_MENU_FILE_SAVE_ALL:
    state = (text_page != NULL);
    break;

  case ID_MENU_FILE_SAVE:
    state = (text_page != NULL && text_page->buffer_modified());
    break;

  default:
    break;
  }

  return state;
}

bool BookFrame::GetEditMenuState(int menu_id) {
  // Always enable these menu items.
  if (menu_id == ID_MENU_EDIT_FIND || menu_id == ID_MENU_EDIT_REPLACE) {
    return true;
  }

  // Let the focused page determine the state.
  BookPage* focused_page = GetFocusedPage();
  if (focused_page != NULL) {
    return focused_page->Page_EditMenuState(menu_id);
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
  bool state = text_page != NULL;

  if (state && check != NULL) {
    switch (menu_id) {
      case ID_MENU_VIEW_WRAP:
        *check = text_page->view_options().wrap;
        break;

      case ID_MENU_VIEW_SHOW_NUMBER:
        *check = text_page->view_options().show_number;
        break;

      case ID_MENU_VIEW_SHOW_SPACE:
        *check = text_page->view_options().show_space;
        break;
    }
  }

  return state;
}

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

  ClearMenuItems(recent_files_menu_);

  int id = 0;
  std::list<wxString>::iterator it = recent_files_.begin();
  for (int i = 0; it != recent_files_.end(); ++it, ++i) {
    id = ID_MENU_FILE_RECENT_FILE_0 + i;
    wxString label = wxString::Format(wxT("%d. "), i) + *it;
    AppendMenuItem(recent_files_menu_, id, label);
  }
}

//------------------------------------------------------------------------------
// Text Page

TextPage* BookFrame::CreateTextPage(editor::TextBuffer* buffer, wxWindow* parent, wxWindowID id) {
  TextPage* page = new TextPage(buffer);

  page->set_style(style_);
  page->set_theme(theme_->GetTheme(THEME_TEXT_PAGE));
  page->set_binding(binding_);

#if JIL_ENABLE_LEADER_KEY
  page->set_leader_key(&leader_key_);
#endif

  page->Create(parent, id, true);

  page->SetTextFont(options_->fonts[FONT_TEXT]);
  page->SetLineNrFont(options_->fonts[FONT_LINE_NR]);
  page->SetLinePadding(options_->line_padding);

  return page;
}

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

void BookFrame::RemoveAllPages(const TextPage* except_page) {
  std::vector<TextPage*> text_pages = text_book_->TextPages();
  
  // If any buffer is modified, ask for save.

  for (TextPage* text_page : text_pages) {
    if (text_page != except_page && text_page->buffer_modified()) {
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

  text_book_->RemoveAllPages(except_page);
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
    text_book_->ActivatePage(text_pages[index]);
  }
}

//------------------------------------------------------------------------------
// File - Open, Save

TextPage* BookFrame::DoOpenFile(const wxString& file_name,
                                bool active,
                                bool silent,
                                bool update_recent_files,
                                bool update_recent_files_menu) {
  wxFileName fn_object(file_name);

  // Make the path absolute, resolve shortcut, etc.
  int flags = wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS | wxPATH_NORM_SHORTCUT;
  fn_object.Normalize(flags);

  bool new_opened = false;
  TextPage* text_page = DoOpenFile(fn_object, active, silent, &new_opened);

  if (text_page != NULL && new_opened) {
    if (update_recent_files) {
      AddRecentFile(fn_object.GetFullPath());
      if (update_recent_files_menu) {
        UpdateRecentFilesMenu();
      }
    }
  }

  return text_page;
}

TextPage* BookFrame::DoOpenFile(const wxFileName& fn_object, bool active, bool silent, bool* new_opened) {
  using namespace editor;

  // Check if this file has already been opened.
  TextPage* text_page = TextPageByFileName(fn_object);
  if (text_page != NULL) {
    if (active) {
      text_book_->ActivatePage(text_page);
    }

    if (new_opened != NULL) {
      *new_opened = false;
    }

    return text_page;
  }

  App& app = wxGetApp();

  const FileType& ft = app.FileTypeFromExt(fn_object.GetExt());
  FtPlugin* ft_plugin = app.GetFtPlugin(ft);

  TextBuffer* buffer = TextBuffer::Create(NewBufferId(),
                                          fn_object,
                                          ft_plugin,
                                          options_->cjk_filters,
                                          options_->file_encoding);
  if (buffer == NULL) {
    if (!silent) {
      // Show error message only when it's not silent.
      wxString msg = wxString::Format(kTrFailedToOpenFile, fn_object.GetFullPath().c_str());
      wxMessageBox(msg, kTrOpenFile, wxOK | wxCENTRE | wxICON_ERROR, this);
    }

    return NULL;
  }

  text_page = CreateTextPage(buffer, text_book_->PageParent(), wxID_ANY);
  text_book_->AddPage(text_page, active);

  if (new_opened != NULL) {
    *new_opened = true;
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

void BookFrame::AddRecentFile(const wxString& recent_file) {
  if (std::find(recent_files_.begin(), recent_files_.end(), recent_file) != recent_files_.end()) {
    return;
  }

  recent_files_.push_front(recent_file);

  while (recent_files_.size() > 10) {
    recent_files_.pop_back();
  }
}

}  // namespace jil
