#include "app/book_frame.h"

#include <fstream>
#include <string>
#include <vector>
#include <list>

#include "wx/sizer.h"
#include "wx/splitter.h"
#include "wx/image.h"
#include "wx/menu.h"
#include "wx/toolbar.h"
#include "wx/filedlg.h"
#include "wx/msgdlg.h"
#include "wx/stdpaths.h"
#include "wx/dir.h"
#include "wx/dnd.h"
#include "wx/utils.h"
#include "wx/clipbrd.h"
#include "wx/dataobj.h"
#include "wx/intl.h"
#include "wx/log.h"

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
#include "editor/status_bar.h"

#include "app/defs.h"
#include "app/app.h"
#include "app/compile_config.h"
#include "app/i18n_strings.h"
#include "app/skin.h"
#include "app/text_page.h"
#include "app/text_book.h"
#include "app/tool_book.h"
#include "app/session.h"
#include "app/config.h"
#include "app/find_window.h"
#include "app/find_result_page.h"
#include "app/splitter.h"
#include "app/save.h"
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

  virtual bool OnDropFiles(wxCoord x,
                           wxCoord y,
                           const wxArrayString& file_names) override {
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
EVT_MENU(wxID_EXIT, BookFrame::OnQuit)
EVT_MENU_RANGE(ID_MENU_EDIT_BEGIN, ID_MENU_EDIT_END - 1, BookFrame::OnMenuEdit)
EVT_MENU_RANGE(ID_MENU_TOOLS_BEGIN, \
               ID_MENU_TOOLS_END - 1, \
               BookFrame::OnMenuTools)
EVT_MENU_RANGE(ID_MENU_HELP_BEGIN, ID_MENU_HELP_END - 1, BookFrame::OnMenuHelp)
// Update UI
EVT_UPDATE_UI_RANGE(ID_MENU_FILE_BEGIN, \
                    ID_MENU_FILE_END - 1, \
                    BookFrame::OnFileUpdateUI)
EVT_UPDATE_UI_RANGE(ID_MENU_EDIT_BEGIN, \
                    ID_MENU_EDIT_END - 1, \
                    BookFrame::OnEditUpdateUI)

EVT_CLOSE(BookFrame::OnClose)

// BookCtrl right click menu.
// NOTE: Only handle "New File"; others are handled by book ctrl.
EVT_MENU(ID_MENU_BOOK_RCLICK_NEW_FILE, BookFrame::OnBookRClickMenu)

// Use wxID_ANY to accept event from any text window.
EVT_TEXT_WINDOW(wxID_ANY, BookFrame::OnTextWindowEvent)

// Status bar.
EVT_STATUS_FIELD_CLICK(ID_STATUS_BAR, BookFrame::OnStatusFieldClick)
// Encoding menu event from status bar.
EVT_MENU_RANGE(ID_MENU_ENCODING_BEGIN, \
               ID_MENU_ENCODING_END - 1, \
               BookFrame::OnStatusEncodingMenu)
EVT_MENU_RANGE(ID_MENU_FILE_FORMAT_BEGIN, \
               ID_MENU_FILE_FORMAT_END - 1, \
               BookFrame::OnStatusFileFormatMenu)
END_EVENT_TABLE()

BookFrame::BookFrame(Options* options, Session* session)
    : options_(options)
    , session_(session)
    , splitter_(NULL)
    , text_books_()  // Zero-initialize
    , tool_book_(NULL)
    , binding_(NULL) {
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

  // Create text books.
  text_books_.resize(1, NULL);  // TODO

  for (size_t i = 0; i < text_books_.size(); ++i) {
    text_books_[i] = new TextBook(theme_->GetTheme(THEME_TEXT_BOOK));
    text_books_[i]->Create(splitter_, wxID_ANY);

    Connect(text_books_[i]->GetId(),
            kEvtBookPageChange,
            wxCommandEventHandler(BookFrame::OnBookPageChange));
    Connect(text_books_[i]->GetId(),
            kEvtBookPageSwitch,
            wxCommandEventHandler(BookFrame::OnBookPageSwitch));
  }

  // Create tool book.
  tool_book_ = new ToolBook(theme_->GetTheme(THEME_TEXT_BOOK));
  tool_book_->Create(splitter_, wxID_ANY);
  tool_book_->Hide();
  Connect(tool_book_->GetId(),
          kEvtBookPageChange,
          wxCommandEventHandler(BookFrame::OnToolBookPageChange));

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
  status_bar_ = new editor::StatusBar;
  status_bar_->set_theme(theme_->GetTheme(THEME_STATUS_BAR));
  status_bar_->SetFields(wxGetApp().status_fields());
  status_bar_->Create(this, ID_STATUS_BAR);

  status_bar_->SetFieldValue(editor::StatusBar::kField_Cwd, wxGetCwd(), false);

  LoadMenus();

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

TextPage* BookFrame::OpenFile(const wxFileName& fn_object,
                              bool active,
                              bool silent) {
  using namespace editor;

  // Check if this file has already been opened.
  TextPage* text_page = TextPageByFileName(fn_object);
  if (text_page != NULL) {
    if (active) {
      ActiveTextBook()->ActivatePage(text_page);
    }
    return text_page;
  }

  App& app = wxGetApp();

  const FileType& ft = app.FileTypeFromExt(fn_object.GetExt());
  FtPlugin* ft_plugin = app.GetFtPlugin(ft);

  TextBuffer* buffer = TextBuffer::Create(fn_object,
                                          ft_plugin,
                                          options_->cjk_filters,
                                          options_->file_encoding);
  if (buffer == NULL) {
    wxString msg = wxString::Format(_("Failed to open file! (%s)"),
                                    fn_object.GetFullPath().c_str());
    if (silent) {
      wxLogError(msg);
    } else {
      wxMessageBox(msg, _("Open File"), wxOK | wxCENTRE | wxICON_ERROR, this);
    }
    return NULL;
  }

  TextBook* text_book = ActiveTextBook();
  text_page = CreateTextPage(buffer, text_book->PageParent());

  text_book->AddPage(text_page, active);

  return text_page;
}

TextPage* BookFrame::OpenFile(const wxString& file_name,
                              bool active,
                              bool silent) {
  wxFileName fn_object(file_name);

  // Make the path absolute, resolve shortcut, etc.
  fn_object.Normalize(wxPATH_NORM_ABSOLUTE |
                      wxPATH_NORM_DOTS |
                      wxPATH_NORM_SHORTCUT);

  return OpenFile(fn_object, active, silent);
}

void BookFrame::OpenFiles(const wxArrayString& file_names, bool silent) {
  if (file_names.IsEmpty()) {
    return;
  }

  ActiveTextBook()->StartBatch();

  // Activate the first opened file.
  bool active = true;
  for (size_t i = 0; i < file_names.size(); ++i) {
    if (OpenFile(file_names[i], active, silent) != NULL) {
      active = false;
    }
  }

  ActiveTextBook()->EndBatch();
}

void BookFrame::FileNew() {
  using namespace editor;

  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(FileType());
  TextBuffer* buffer = TextBuffer::Create(ft_plugin, options_->file_encoding);

  TextBook* text_book = ActiveTextBook();
  TextPage* text_page = CreateTextPage(buffer, text_book->PageParent());
  text_book->AddPage(text_page, true);
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
  ActiveTextBook()->RemoveActivePage();
}

void BookFrame::FileCloseAll() {
  RemoveAllPages();
}

void BookFrame::FileSave() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer != NULL) {
    DoSaveBuffer(buffer);
  }
}

void BookFrame::FileSaveAs() {
  editor::TextBuffer* buffer = ActiveBuffer();
  if (buffer != NULL) {
    SaveBufferAs(buffer, this);
  }
}

void BookFrame::FileSaveAll() {
  for (size_t i = 0; i < text_books_.size(); ++i) {
    std::vector<TextPage*> text_pages = BookTextPages(text_books_[i]);

    for (size_t j = 0; j < text_pages.size(); ++j) {
      if (text_pages[j] != NULL) {
        DoSaveBuffer(text_pages[j]->buffer());
      }
    }
  }
}

//------------------------------------------------------------------------------

size_t BookFrame::PageCount() const {
  return ActiveTextBook()->PageCount();
}

void BookFrame::SwitchToNextPage() {
  ActiveTextBook()->SwitchToNextPage();
}

void BookFrame::SwitchToPrevPage() {
  ActiveTextBook()->SwitchToPrevPage();
}

void BookFrame::SwitchToNextStackPage() {
  // TODO
}

void BookFrame::SwitchToPrevStackPage() {
  // TODO
}

void BookFrame::ShowFind() {
  ShowFindWindow(::jil::FindWindow::kFindMode);
}

void BookFrame::ShowReplace() {
  ShowFindWindow(::jil::FindWindow::kReplaceMode);
}

TextPage* BookFrame::ActiveTextPage() const {
  BookPage* page = ActiveTextBook()->ActivePage();
  if (page == NULL) {
    return NULL;
  }

  if (page->Page_Window()->IsBeingDeleted()) {
    return NULL;
  }

  return AsTextPage(page);
}

editor::TextBuffer* BookFrame::ActiveBuffer() const {
  TextPage* page = ActiveTextPage();
  if (page == NULL) {
    return NULL;
  }
  return page->buffer();
}

void BookFrame::FindInActivePage(const std::wstring& str, int flags) {
  using namespace editor;

  TextPage* text_page = ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  TextPoint point = text_page->caret_point();

  if (GetBit(flags, kFindReversely)) {
    // If there's any selected text, it might be the last find result. We don't
    // check it (for simplicity). But the find start point has to be adjusted,
    // otherwise the find result will always be this selected text.
    if (!text_page->selection().IsEmpty()) {
      point = text_page->selection().begin();
    }
  }

  TextRange result_range = Find(text_page, str, point, flags, true);
  if (result_range.IsEmpty()) {
    return;
  }

  text_page->SetSelection(result_range, kForward, false);
  // Don't scroll right now, scroll later.
  text_page->UpdateCaretPoint(result_range.point_end(), false, false, false);

  // Always scroll to the begin point of the result range. This makes more
  // sense especially for multiple-line result range (regex find only).
  // Don't use ScrollToPoint().
  // TODO: Horizontally scroll if necessary.
  text_page->Goto(result_range.line_first());
}

#if 0
void BookFrame::FindInAllPages(const std::wstring& str, int flags) {
  using namespace editor;

  TextBook* text_book = ActiveTextBook();

  TextPage* text_page = text_book->ActiveTextPage();
  if (text_page == NULL) {
    return;
  }

  // Find in active text page.
  TextRange result_range = Find(text_page,
                                str,
                                text_page->caret_point(),
                                flags,
                                false);

  if (!result_range.IsEmpty()) {
    // Select the matched text.
    if (GetBit(flags, kFindReversely)) {
      text_page->SetSelection(result_range, kBackward);
    } else {
      text_page->SetSelection(result_range, kForward);
    }
    return;
  }

  TextPage* start_text_page = text_page;

  do {
    // Go to next page.
    text_page = AsTextPage(text_book->NextPage(text_page));

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

  if (result_range.IsEmpty()) {  // No match
    return;
  }

  if (text_page == start_text_page) {
    return;
  }

  // Activate the text page where the match is found.
  text_book->ActivatePage(text_page);

  // Select the matched text.
  if (GetBit(flags, kFindReversely)) {
    text_page->SetSelection(result_range, kBackward);
  } else {
    text_page->SetSelection(result_range, kForward);
  }
}
#endif  // 0

void BookFrame::FindAllInActivePage(const std::wstring& str, int flags) {
  TextPage* fr_page = GetFindResultPage();

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

#if 0
void BookFrame::FindAllInAllPages(const std::wstring& str, int flags) {
  TextPage* fr_page = GetFindResultPage();

  // Clear previous result.
  editor::TextRange range = fr_page->buffer()->range();
  if (!range.IsEmpty()) {
    fr_page->buffer()->DeleteText(range);
  }
  fr_page->buffer()->Line(1)->set_id(editor::kNpos); // Clear line ID

  std::vector<TextPage*> text_pages = BookTextPages(ActiveTextBook());
  for (TextPage* text_page : text_pages) {
    FindAll(str, text_page->buffer(), flags, fr_page);
  }

  // Reset caret point.
  fr_page->UpdateCaretPoint(kPointBegin, false, true, false);

  ActivateToolPage(fr_page);
}
#endif  // 0

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
    text_page->SetSelection(result_range, kForward, false);
    text_page->UpdateCaretPoint(result_range.point_end(), false, false, false);
    text_page->Goto(result_range.line_first());
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
    text_page->SetSelection(result_range, kForward, false);
    text_page->UpdateCaretPoint(result_range.point_end(), false, false, false);
    text_page->Goto(result_range.line_first());
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

  bool use_regex = GetBit(flags, kFindUseRegex);
  bool case_sensitive = GetBit(flags, kFindCaseSensitive);
  bool match_whole_word = GetBit(flags, kFindMatchWholeWord);

  TextRange source_range = buffer->range();
  TextRange result_range;

  size_t count = 0;

  while (true) {
    result_range = buffer->FindString(str,
                                      source_range,
                                      use_regex,
                                      case_sensitive,
                                      match_whole_word,
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
      text_page->InsertString(result_range.point_begin(),
                              replace_str,
                              false,
                              false);
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

  bool use_regex = GetBit(flags, kFindUseRegex);
  bool case_sensitive = GetBit(flags, kFindCaseSensitive);
  bool match_whole_word = GetBit(flags, kFindMatchWholeWord);
  // Reversely regex find is not supported.
  bool reversely = !use_regex && GetBit(flags, kFindReversely);

  if (reversely) {
    source_range.Set(buffer->point_begin(), point);
  } else {
    source_range.Set(point, buffer->point_end());
  }

  TextRange result_range = buffer->FindString(str,
                                              source_range,
                                              use_regex,
                                              case_sensitive,
                                              match_whole_word,
                                              reversely);

  if (result_range.IsEmpty() && cycle) {
    bool refind = false;

    if (reversely) {
      // Re-find from the end.
      Coord last_line_len = buffer->LineLength(buffer->LineCount());
      if (point.y < buffer->LineCount() ||
          point.x + CoordCast(str.length()) <= last_line_len) {
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
      result_range = buffer->FindString(str,
                                        source_range,
                                        use_regex,
                                        case_sensitive,
                                        match_whole_word,
                                        reversely);
    }
  }

  return result_range;
}

// Example: "%6d", *max_ln_size = 6
static std::wstring CreateLineNrStrFormat(const TextRangeList& result_ranges,
                                          size_t* max_ln_size) {
  editor::Coord max_ln = result_ranges.back().point_end().y;

  std::string max_ln_str = base::LexicalCast<std::string>(max_ln);
  *max_ln_size = max_ln_str.size();

  std::wstring max_ln_size_str = base::LexicalCast<std::wstring>(
      max_ln_str.size());
  return (L"%" + max_ln_size_str + L"d");
}

void BookFrame::FindAll(const std::wstring& str,
                        editor::TextBuffer* buffer,
                        int flags,
                        TextPage* fr_page) {
  using namespace editor;

  std::list<TextRange> result_ranges;
  buffer->FindStringAll(str,
                        buffer->range(),
                        GetBit(flags, kFindUseRegex),
                        GetBit(flags, kFindCaseSensitive),
                        GetBit(flags, kFindMatchWholeWord),
                        &result_ranges);

  if (result_ranges.empty()) {
    return;
  }

  TextLine* fr_line = NULL;

  // Add file path name line.
  std::wstring file_path_name = buffer->file_path_name().ToStdWstring();
  file_path_name = L"-- " + file_path_name;
  fr_line = fr_page->buffer()->AppendLine(file_path_name);
  fr_line->set_id(kNpos);  // Clear line ID.

  size_t ln_size = 1;
  std::wstring ln_str_format = CreateLineNrStrFormat(result_ranges, &ln_size);
  std::wstring ln_str_buf;
  ln_str_buf.resize(ln_size);

  std::list<TextRange>::iterator range_it = result_ranges.begin();
  for (; range_it != result_ranges.end(); ++range_it) {
    const TextRange& range = *range_it;

    Coord ln = range.point_begin().y;
    swprintf(&ln_str_buf[0], ln_str_format.c_str(), ln);

    TextLine* line = buffer->Line(ln);

    std::wstring line_data = ln_str_buf + L" " + line->data();
    fr_line = fr_page->buffer()->AppendLine(line_data);

    // Reuse the ID of the source line.
    // This might make the line IDs in find result page not unique.
    // Note that other lines are all set kNpos (-1) as ID.
    fr_line->set_id(line->id());

    // Add lex element for the prefix line number.
    fr_line->AddLexElement(0,
                           ln_str_buf.size(),
                           Lex(kLexConstant, kLexConstantNumber));

    // Add lex element for the matched string.
    // TODO: Multiple line match when using regex.
    size_t off = range.point_begin().x + ln_str_buf.size() + 1;
    size_t len = range.point_end().x - range.point_begin().x;
    fr_line->AddLexElement(off, len, Lex(kLexIdentifier));
  }

  // Add match count line.
  // Example: >> 4
  std::wstring match_count = L">> ";
  match_count += base::LexicalCast<std::wstring>(result_ranges.size());
  fr_line = fr_page->buffer()->AppendLine(match_count);
  fr_line->set_id(kNpos);  // Clear line ID.
}

//------------------------------------------------------------------------------

void BookFrame::OnSize(wxSizeEvent& evt) {
  if (GetClientSize().x < editor::kUnreadyWindowSize) {
    return;
  }

  UpdateLayout();
}

void BookFrame::UpdateLayout() {
  const wxRect client_rect = GetClientRect();

  const int status_height = status_bar_->GetBestSize().y;
  const int book_area_height = client_rect.GetHeight() - status_height;

  splitter_->SetSize(0, 0, client_rect.GetWidth(), book_area_height);
  splitter_->Split();

  status_bar_->SetSize(0,
                       book_area_height,
                       client_rect.GetWidth(),
                       status_height);
}

void BookFrame::RestoreSplitTree(SplitNode* n) {
  SplitLeaf* leaf = n->AsLeaf();
  if (leaf != NULL) {
    if (leaf->id() == kSplit_Text) {
      leaf->set_window(text_books_[0]);
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

  SplitLeaf* text_node = new SplitLeaf(text_books_[0]);
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

// NOTE: Don't match text command because you don't know what the current
// text window is. Instead, return false and let the system delegate key down
// event to the current text window.
bool BookFrame::HandleKeyDownHook(wxKeyEvent& evt) {
  int code = evt.GetKeyCode();
  int modifiers = evt.GetModifiers();

  if (code == WXK_NONE) {
    return false;
  }
  if (code == WXK_CONTROL || code == WXK_SHIFT || code == WXK_ALT) {
    return false;
  }

  if (code == WXK_ESCAPE) {
    if (!leader_key_.IsEmpty()) {
      leader_key_.Reset();
      status_bar_->SetFieldValue(editor::StatusBar::kField_KeyStroke,
                                 wxEmptyString,
                                 true);
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
      if (editor::kNonShiftChars.find(static_cast<char>(code))
          != std::string::npos) {
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
  if (void_func != NULL && (menu == 0 || GetMenuEnableState(menu))) {
    // Clear leader key before execute the function.
    if (!leader_key_.IsEmpty()) {
      leader_key_.Reset();
      status_bar_->SetFieldValue(editor::StatusBar::kField_KeyStroke,
                                 wxEmptyString,
                                 true);
    }

    void_func->Exec();
    return true;
  }

  // No void command matched.
  // Return false to let current text window handle the event.

  // If a text command is matched in the current text window, the leader key
  // will also be reset if it's not empty (An event will be posted from the
  // text window for this). See OnTextWindowEvent().

  return false;
}

void BookFrame::OnMenuFile(wxCommandEvent& evt) {
  int menu = evt.GetId();
  editor::VoidFunc* void_func = binding_->GetVoidFuncByMenu(menu);
  if (void_func != NULL) {
    void_func->Exec();
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

void BookFrame::OnMenuTools(wxCommandEvent& evt) {
  switch (evt.GetId()) {
    case ID_MENU_TOOLS_OPTIONS: {
      // TODO
      break;
    }
  }
}

//------------------------------------------------------------------------------

void BookFrame::OnMenuHelp(wxCommandEvent& evt) {
  switch (evt.GetId()) {
    case ID_MENU_HELP_VIEW_ONLINE:
      wxMessageBox(kTrComingSoon);
      break;

    case ID_MENU_HELP_ABOUT:
      wxMessageBox(kTrComingSoon);
      break;
  }
}

//------------------------------------------------------------------------------

bool BookFrame::ExecFuncByMenu(int menu) {
  // Match text command.
  TextPage* page = ActiveTextPage();
  if (page != NULL) {
    editor::TextFunc* text_func = binding_->GetTextFuncByMenu(menu);
    if (text_func != NULL) {
      text_func->Exec(page);
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
  bool state = GetFileMenuEnableState(menu_id);
  evt.Enable(state);
}

void BookFrame::OnEditUpdateUI(wxUpdateUIEvent& evt) {
  int menu_id = evt.GetId();
  bool state = GetEditMenuEnableState(menu_id);
  evt.Enable(state);
}

void BookFrame::OnClose(wxCloseEvent& evt) {
  // Remember opened files.
  std::list<wxString> opened_files;
  wxString active_file;

  TextBook* text_book = ActiveTextBook();
  if (text_book->PageCount() > 0) {
    std::vector<TextPage*> text_pages = BookTextPages(text_book);

    for (size_t i = 0; i < text_pages.size(); ++i) {
      if (!text_pages[i]->buffer()->new_created()) {
        opened_files.push_back(text_pages[i]->buffer()->file_path_name());
      }
    }

    TextPage* text_page = text_book->ActiveTextPage();
    if (text_page != NULL) {
      if (!text_page->buffer()->new_created()) {
        active_file = text_page->buffer()->file_path_name();
      }
    }
  }

  RemoveAllPages();

  if (PageCount() > 0) {
    return;
  }

  // Save session.

  session_->set_last_opened_files(opened_files);
  session_->set_last_active_file(active_file);

  if (IsMaximized()) {
    session_->set_book_frame_maximized(true);
  } else {
    session_->set_book_frame_maximized(false);
    session_->set_book_frame_rect(GetScreenRect());
  }

  ::jil::FindWindow* find_window = GetFindWindow();
  if (find_window != NULL) {
    session_->set_find_window_rect(find_window->GetScreenRect());
    session_->set_find_flags(find_window->flags());
  }

  evt.Skip();
}

// NOTE: Only handle "New File"; others are handled by book ctrl.
void BookFrame::OnBookRClickMenu(wxCommandEvent& evt) {
  if (evt.GetId() == ID_MENU_BOOK_RCLICK_NEW_FILE) {
    FileNew();
  }
}

void BookFrame::OnBookPageChange(wxCommandEvent& evt) {
  // Clear status fields if all pages are removed.
  if (ActiveTextBook()->PageCount() == 0) {
    UpdateStatusFields();
    UpdateTitle();

    // Close find window.
    ::jil::FindWindow* find_window = GetFindWindow();
    if (find_window != NULL) {
      find_window->Close();
    }
  }
}

void BookFrame::OnBookPageSwitch(wxCommandEvent& evt) {
  UpdateStatusFields();
  UpdateTitle();
}

void BookFrame::OnToolBookPageChange(wxCommandEvent& evt) {
  // If tool book has no page, hide it.
  if (tool_book_->PageCount() == 0) {
    if (tool_book_->IsShown()) {
      tool_book_->Hide();
      splitter_->Split();  // Update layout.

      // Transfer focus to text book.
      ActiveTextBook()->SetFocus();
    }
  }
}

void BookFrame::UpdateStatusFields() {
  using namespace editor;

  TextPage* text_page = AsTextPage(ActiveTextBook()->ActivePage());

  if (text_page == NULL) {
    status_bar_->ClearFieldValues();
  } else {
    // Update field values.
    TextBuffer* buffer = text_page->buffer();

    if (options_->switch_cwd && !buffer->new_created()) {
      wxString cwd = buffer->file_path(wxPATH_GET_VOLUME, wxPATH_NATIVE);
      wxSetWorkingDirectory(cwd);
      status_bar_->SetFieldValue(StatusBar::kField_Cwd, cwd, false);
    }

    status_bar_->SetFieldValue(StatusBar::kField_Encoding,
                               buffer->file_encoding().display_name,
                               false);
    status_bar_->SetFieldValue(StatusBar::kField_FileFormat,
                               FileFormatName(buffer->file_format()),
                               false);
    status_bar_->SetFieldValue(StatusBar::kField_FileType,
                               buffer->ft_plugin()->name(),
                               false);
    status_bar_->SetFieldValue(StatusBar::kField_Caret,
                               FormatCaretString(text_page),
                               false);
  }

  status_bar_->UpdateFieldSizes();
  status_bar_->Refresh();
}

void BookFrame::UpdateTitle() {
  TextPage* text_page = AsTextPage(ActiveTextBook()->ActivePage());
  if (text_page == NULL) {
    SetTitle(kAppDisplayName);
  } else {
    wxString title = text_page->Page_Description() +
                     wxT(" - ") +
                     kAppDisplayName;
    SetTitle(title);
  }
}

wxString BookFrame::FormatCaretString(TextPage* page) const {
  wxString format = wxT("%d/%d, %d"); // TODO
  return wxString::Format(format,
                          page->caret_point().y,
                          page->buffer()->LineCount(),
                          page->caret_point().x);
}

// Update status bar according to the event.
void BookFrame::OnTextWindowEvent(wxCommandEvent& evt) {
  int type = evt.GetInt();

  if (type == editor::TextWindow::kEncodingEvent) {
    wxString encoding;

    TextPage* text_page = ActiveTextPage();
    if (text_page != NULL) {
      encoding = text_page->buffer()->file_encoding().display_name;
    }

    status_bar_->SetFieldValue(editor::StatusBar::kField_Encoding,
                               encoding,
                               false);

    status_bar_->UpdateFieldSizes();
    status_bar_->Refresh();

  } else if (type == editor::TextWindow::kCaretEvent) {
    TextPage* text_page = ActiveTextPage();
    if (text_page != NULL) {
      status_bar_->SetFieldValue(editor::StatusBar::kField_Caret,
                                 FormatCaretString(text_page),
                                 true);
    }
  } else if (type == editor::TextWindow::kLeaderKeyEvent) {
    // Leader key is reset by the text window.
    if (leader_key_.IsEmpty()) {  // Must be empty but just check it.
      status_bar_->SetFieldValue(editor::StatusBar::kField_KeyStroke,
                                 wxEmptyString,
                                 true);
    }
  } else if (type == editor::TextWindow::kFileFormatEvent) {
    TextPage* text_page = ActiveTextPage();
    if (text_page != NULL) {
      editor::FileFormat ff = text_page->buffer()->file_format();
      status_bar_->SetFieldValue(editor::StatusBar::kField_FileFormat,
                                 editor::FileFormatName(ff),
                                 true);
    }
  }
}

void BookFrame::OnFindResultPageEvent(wxCommandEvent& evt) {
  using namespace editor;

  FindResultPage* fr_page = GetFindResultPage();
  if (fr_page == NULL) {
    return;
  }

  int type = evt.GetInt();

  if (type == FindResultPage::kLocalizeEvent) {
    Coord caret_y = fr_page->caret_point().y;

    TextBuffer* fr_buffer = fr_page->buffer();

    // Find the file path line.
    // NOTE: Checking line prefix is a temp solution.
    const std::wstring kPrefix = L"-- ";

    wxString file_path;
    for (Coord ln = caret_y - 1; ln > 0; --ln) {
      if (fr_buffer->Line(ln)->StartWith(kPrefix, false)) {
        file_path = fr_buffer->LineData(ln).substr(kPrefix.size());
        break;
      }
    }

    if (file_path.IsEmpty()) {
      return;
    }

    // Find the source text page.
    wxFileName fn_object(file_path);
    TextPage* text_page = TextPageByFileName(fn_object);
    if (text_page == NULL) {
      // The page is closed? Reopen it.
      text_page = OpenFile(fn_object, true, false);
      if (text_page == NULL) {
        return;  // Failed to reopen it.
      }
    } else {
      // The page might not be active, activate it.
      ActiveTextBook()->ActivatePage(text_page);
    }

    size_t line_id = fr_buffer->Line(caret_y)->id();

    Coord ln = text_page->buffer()->LineNrFromId(line_id);
    if (ln == kInvalidCoord) {
      return;
    }

    // Go to the source line.
    text_page->Goto(ln);
  }
}

void BookFrame::OnStatusFieldClick(wxCommandEvent& evt) {
  using namespace editor;

  TextBuffer* active_buffer = ActiveBuffer();
  if (active_buffer == NULL) {
    return;
  }

  int field_id = evt.GetInt();

  switch (field_id) {
  case StatusBar::kField_Encoding:
    PopupEncodingMenu();
    break;

  case StatusBar::kField_FileFormat:
    PopupFileFormatMenu();
    break;

  default:
    break;
  }
}

static bool IsTraditionalChinese(int lang) {
  return (lang >= wxLANGUAGE_CHINESE_TRADITIONAL &&
          lang <= wxLANGUAGE_CHINESE_TAIWAN);
}

// Use sub-menus to simplified the first level menu items.
// Dynamically adjust the first level menu items according to
// the current locale.
void BookFrame::PopupEncodingMenu() {
  wxLocale locale;
  locale.Init();
  int lang = locale.GetLanguage();

  wxMenu menu;

  // UNICODE encodings.
  menu.Append(ID_MENU_ENCODING_UTF8, ENCODING_DISPLAY_NAME_UTF8);
  menu.Append(ID_MENU_ENCODING_UTF8_BOM, ENCODING_DISPLAY_NAME_UTF8_BOM);
  menu.Append(ID_MENU_ENCODING_UTF16_BE, ENCODING_DISPLAY_NAME_UTF16_BE);
  menu.Append(ID_MENU_ENCODING_UTF16_LE, ENCODING_DISPLAY_NAME_UTF16_LE);

  // Language specific encodings.
  if (lang == wxLANGUAGE_CHINESE_SIMPLIFIED) {
    menu.Append(ID_MENU_ENCODING_GB18030, ENCODING_DISPLAY_NAME_GB18030);
  } else if (IsTraditionalChinese(lang)) {
    menu.Append(ID_MENU_ENCODING_BIG5, ENCODING_DISPLAY_NAME_BIG5);
  } else if (lang == wxLANGUAGE_JAPANESE) {
    menu.Append(ID_MENU_ENCODING_SHIFT_JIS, ENCODING_DISPLAY_NAME_SHIFT_JIS);
    menu.Append(ID_MENU_ENCODING_EUC_JP, ENCODING_DISPLAY_NAME_EUC_JP);
  }

  wxMenu* sub_menu = new wxMenu;

  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_1,
                   ENCODING_DISPLAY_NAME_ISO_8859_1);

  sub_menu->Append(ID_MENU_ENCODING_WINDOWS_1250,
                   ENCODING_DISPLAY_NAME_WINDOWS_1250);
  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_2,
                   ENCODING_DISPLAY_NAME_ISO_8859_2);

  if (lang != wxLANGUAGE_CHINESE_SIMPLIFIED) {
    sub_menu->Append(ID_MENU_ENCODING_GB18030, ENCODING_DISPLAY_NAME_GB18030);
  }
  if (!IsTraditionalChinese(lang)) {
    sub_menu->Append(ID_MENU_ENCODING_BIG5, ENCODING_DISPLAY_NAME_BIG5);
  }

  if (lang != wxLANGUAGE_JAPANESE) {
    sub_menu->Append(ID_MENU_ENCODING_SHIFT_JIS,
                     ENCODING_DISPLAY_NAME_SHIFT_JIS);
    sub_menu->Append(ID_MENU_ENCODING_EUC_JP, ENCODING_DISPLAY_NAME_EUC_JP);
  }

  sub_menu->Append(ID_MENU_ENCODING_TIS_620, ENCODING_DISPLAY_NAME_TIS_620);

  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_5,
                   ENCODING_DISPLAY_NAME_ISO_8859_5);
  sub_menu->Append(ID_MENU_ENCODING_KOI8_R, ENCODING_DISPLAY_NAME_KOI8_R);
  sub_menu->Append(ID_MENU_ENCODING_MAC_CYRILLIC,
                   ENCODING_DISPLAY_NAME_X_MAC_CYRILLIC);
  sub_menu->Append(ID_MENU_ENCODING_WINDOWS_1251,
                   ENCODING_DISPLAY_NAME_WINDOWS_1251);

  sub_menu->Append(ID_MENU_ENCODING_ISO_8859_7,
                   ENCODING_DISPLAY_NAME_ISO_8859_7);
  sub_menu->Append(ID_MENU_ENCODING_WINDOWS_1253,
                   ENCODING_DISPLAY_NAME_WINDOWS_1253);

  menu.AppendSubMenu(sub_menu, _("Others..."));

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::PopupFileFormatMenu() {
  wxMenu menu;
  menu.Append(ID_MENU_FILE_FORMAT_WIN, FF_DIAPLAY_NAME_WIN);
  menu.Append(ID_MENU_FILE_FORMAT_UNIX, FF_DIAPLAY_NAME_UNIX);
  menu.Append(ID_MENU_FILE_FORMAT_MAC, FF_DIAPLAY_NAME_MAC);
  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void BookFrame::OnStatusEncodingMenu(wxCommandEvent& evt) {
  using namespace editor;

  TextBuffer* active_buffer = ActiveBuffer();
  if (active_buffer == NULL) {
    return;
  }

  // NOTE: Keep consistent with enum EncodingMenuId.
  static const std::string kEncodingNames[] = {
    ENCODING_NAME_UTF8,
    ENCODING_NAME_UTF8_BOM,
    ENCODING_NAME_UTF16_BE,
    ENCODING_NAME_UTF16_LE,
    ENCODING_NAME_GB18030,
    ENCODING_NAME_BIG5,
    ENCODING_NAME_SHIFT_JIS,
    ENCODING_NAME_EUC_JP,
    ENCODING_NAME_KOI8_R,
    ENCODING_NAME_ISO_8859_1,
    ENCODING_NAME_ISO_8859_2,
    ENCODING_NAME_ISO_8859_5,
    ENCODING_NAME_ISO_8859_7,
    ENCODING_NAME_TIS_620,
    ENCODING_NAME_WINDOWS_1250,
    ENCODING_NAME_WINDOWS_1251,
    ENCODING_NAME_WINDOWS_1253,
    ENCODING_NAME_X_MAC_CYRILLIC,
  };

  int index = evt.GetId() - ID_MENU_ENCODING_BEGIN;
  if (index >= (sizeof(kEncodingNames) / sizeof(std::string))) {
    return;
  }

  Encoding encoding = EncodingFromName(kEncodingNames[index]);

  active_buffer->set_file_encoding(encoding);
  active_buffer->Notify(kEncodingChange);
}

void BookFrame::OnStatusFileFormatMenu(wxCommandEvent& evt) {
  using namespace editor;

  TextPage* active_page = ActiveTextPage();
  if (active_page == NULL) {
    return;
  }

  FileFormat ff = FF_NONE;

  switch (evt.GetId()) {
  case ID_MENU_FILE_FORMAT_WIN:
    ff = FF_WIN;
    break;

  case ID_MENU_FILE_FORMAT_UNIX:
    ff = FF_UNIX;
    break;

  case ID_MENU_FILE_FORMAT_MAC:
    ff = FF_MAC;
    break;
  }

  if (ff != FF_NONE) {
    active_page->SetFileFormat(ff);
  }
}

::jil::FindWindow* BookFrame::GetFindWindow() const {
  wxWindow* w = FindWindowById(ID_FIND_WINDOW, this);
  if (w == NULL) {
    return NULL;
  } else {
    return wxDynamicCast(w, ::jil::FindWindow);
  }
}

void BookFrame::ShowFindWindow(int find_window_mode) {
  const int kFindDefaultWidth = 300;

  wxRect rect = session_->find_window_rect();
  if (rect.IsEmpty()) {
    // Determine find window rect according to client rect.
    wxRect client_rect = GetClientRect();
    client_rect.SetLeftTop(ClientToScreen(client_rect.GetLeftTop()));
    rect = wxRect(client_rect.GetRight() - kFindDefaultWidth,
                  client_rect.GetTop(),
                  kFindDefaultWidth,
                  -1);
  } else {
    rect.SetHeight(-1);
  }

  ::jil::FindWindow* find_window = GetFindWindow();
  if (find_window == NULL) {
    find_window = new ::jil::FindWindow(session_, find_window_mode);
    find_window->Create(this, ID_FIND_WINDOW);
  } else {
    find_window->set_mode(find_window_mode);
    find_window->UpdateLayout();
  }

  if (!find_window->IsShown()) {
    find_window->SetSize(rect);
    find_window->Show();
  }

  // Find the selected text.
  TextPage* text_page = ActiveTextPage();
  if (text_page != NULL && !text_page->selection().IsEmpty()) {
    editor::TextRange select_range = text_page->selection().range;

    if (select_range.LineCount() == 1) {
      // If there is any text selected in the active page and the selection is
      // inside a single line, it might be what the user wants to find.
      std::wstring find_string;
      text_page->buffer()->GetText(select_range, &find_string);
      find_window->SetFindString(wxString(find_string.c_str()));
    }
  }

  // Activate it.
  find_window->Raise();
}

// Load file type plugin for find result buffer.
static void LoadFindResultFtPlugin(editor::FtPlugin* ft_plugin) {
  using namespace editor;

  ft_plugin->AddQuote(new Quote(kLexComment, L"--", L"", 0));
  ft_plugin->AddQuote(new Quote(kLexComment, L">>", L"", 0));

  // The text inside find result page all starts with a line number. If the
  // page itself also displays line numbers, it won't be clear for the user
  // to view the find result.
  ft_plugin->options().show_number = false;
}

FindResultPage* BookFrame::GetFindResultPage() {
  using namespace editor;

  wxWindow* w = FindWindowById(ID_FIND_RESULT_PAGE, tool_book_);

  if (w != NULL) {
    FindResultPage* page = wxDynamicCast(w, FindResultPage);
    assert(page != NULL);
    return page;
  }

  FileType ft(kFtId_FindResult, wxEmptyString);
  FtPlugin* ft_plugin = wxGetApp().GetFtPlugin(ft,
                                               LoadFindResultFtPlugin);
  TextBuffer* buffer = TextBuffer::Create(ft_plugin,
                                          options_->file_encoding);

  FindResultPage* fr_page = new FindResultPage(buffer);

  fr_page->set_style(style_);
  fr_page->set_theme(theme_->GetTheme(THEME_TEXT_PAGE));
  fr_page->set_binding(binding_);
  fr_page->set_leader_key(&leader_key_);

  fr_page->Create(tool_book_->PageParent(), ID_FIND_RESULT_PAGE, true);

  fr_page->SetTextFont(options_->font);
  fr_page->SetLineNrFont(options_->font);

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

//------------------------------------------------------------------------------

wxMenuItem* BookFrame::NewMenuItem(wxMenu* menu,
                                   int id,
                                   const wxString& label,
                                   wxItemKind item_kind) {
  wxString menu_label = label;

  // Append accelerator if any.
  editor::Key menu_key = binding_->GetKeyByMenu(id);
  if (!menu_key.IsEmpty()) {
    menu_label += wxT("\t") + menu_key.ToString();
  }

  wxMenuItem* menu_item =
      new wxMenuItem(menu, id, menu_label, wxEmptyString, item_kind);
  menu->Append(menu_item);

  return menu_item;
}

void BookFrame::LoadMenus() {
  wxMenuBar* menu_bar = new wxMenuBar;

  //------------------------------------
  // File

  wxMenu* menu_file = new wxMenu;
  // - New
  NewMenuItem(menu_file, ID_MENU_FILE_NEW, kTrFileNew);

#if JIL_MULTIPLE_WINDOW
  menu_file->AppendSeparator();
  NewMenuItem(menu_file, ID_MENU_FILE_NEW_WINDOW, kTrFileNewFrame);
#endif  // JIL_MULTIPLE_WINDOW

  // - Open
  menu_file->AppendSeparator();
  NewMenuItem(menu_file, ID_MENU_FILE_OPEN, kTrFileOpen);
  menu_file->AppendSeparator();
  // - Close
  NewMenuItem(menu_file, ID_MENU_FILE_CLOSE, kTrFileClose);
  NewMenuItem(menu_file, ID_MENU_FILE_CLOSE_ALL, kTrFileCloseAll);
  menu_file->AppendSeparator();
  NewMenuItem(menu_file, ID_MENU_FILE_SAVE, kTrFileSave);
  NewMenuItem(menu_file, ID_MENU_FILE_SAVE_AS, kTrFileSaveAs);
  NewMenuItem(menu_file, ID_MENU_FILE_SAVE_ALL, kTrFileSaveAll);
  menu_file->AppendSeparator();
  // - Exit
  NewMenuItem(menu_file, wxID_EXIT, kTrFileExit);  // TODO: ID

  menu_bar->Append(menu_file, kTrMenuFile);

  //------------------------------------
  // Edit

  wxMenu* menu_edit = new wxMenu;

  NewMenuItem(menu_edit, ID_MENU_EDIT_UNDO, kTrEditUndo);
  NewMenuItem(menu_edit, ID_MENU_EDIT_REDO, kTrEditRedo);
  menu_edit->AppendSeparator();

  NewMenuItem(menu_edit, ID_MENU_EDIT_CUT, kTrEditCut);
  NewMenuItem(menu_edit, ID_MENU_EDIT_COPY, kTrEditCopy);
  NewMenuItem(menu_edit, ID_MENU_EDIT_PASTE, kTrEditPaste);
  menu_edit->AppendSeparator();

  // - Line
  wxMenu* menu_line = new wxMenu;
  NewMenuItem(menu_line, ID_MENU_EDIT_AUTO_INDENT, kTrEditAutoIndent);
  NewMenuItem(menu_line, ID_MENU_EDIT_INCREASE_INDENT, kTrEditIncreaseIndent);
  NewMenuItem(menu_line, ID_MENU_EDIT_DECREASE_INDENT, kTrEditDecreaseIndent);
  menu_edit->AppendSubMenu(menu_line, kTrEditLine);

  // - Comment
  wxMenu* menu_comment = new wxMenu;
  NewMenuItem(menu_comment, ID_MENU_EDIT_COMMENT, kTrEditComment);
  NewMenuItem(menu_comment, ID_MENU_EDIT_UNCOMMENT, kTrEditUncomment);
  NewMenuItem(menu_comment, ID_MENU_EDIT_TOGGLE_COMMENT, kTrEditToggleComment);
  menu_edit->AppendSubMenu(menu_comment, kTrEditComment);

  NewMenuItem(menu_edit, ID_MENU_EDIT_FORMAT, kTrEditFormat);
  menu_edit->AppendSeparator();

  NewMenuItem(menu_edit, ID_MENU_EDIT_FIND, kTrEditFind);
  NewMenuItem(menu_edit, ID_MENU_EDIT_REPLACE, kTrEditReplace);
  NewMenuItem(menu_edit, ID_MENU_EDIT_GOTO, kTrEditGoto);

  menu_bar->Append(menu_edit, kTrMenuEdit);

  //------------------------------------
  // Tools

  wxMenu* menu_tools = new wxMenu;
  menu_tools->Append(ID_MENU_TOOLS_OPTIONS, kTrToolsOptions);
  menu_bar->Append(menu_tools, kTrMenuTools);

  //------------------------------------
  // Help

  wxMenu* menu_Help = new wxMenu;
  menu_Help->Append(ID_MENU_HELP_VIEW_ONLINE, kTrHelpViewOnline);
  menu_Help->Append(ID_MENU_HELP_ABOUT, kTrHelpAbout);
  menu_bar->Append(menu_Help, kTrMenuHelp);

  SetMenuBar(menu_bar);
}

bool BookFrame::GetFileMenuEnableState(int menu_id) const {
  using namespace editor;

  TextPage* page = ActiveTextPage();
  TextBuffer* buffer = page == NULL ? NULL : page->buffer();

  bool state = true;

  switch (menu_id) {
  case ID_MENU_FILE_CLOSE:
    // Fall through.
  case ID_MENU_FILE_CLOSE_ALL:
    state = (page != NULL);
    break;

  case ID_MENU_FILE_SAVE:
    state = (buffer != NULL && buffer->modified());
    break;

  case ID_MENU_FILE_SAVE_AS:
    // Fall through.
  case ID_MENU_FILE_SAVE_ALL:
    state = (buffer != NULL);
    break;

  default:
    break;
  }

  return state;
}

bool BookFrame::GetEditMenuEnableState(int menu_id) const {
  TextPage* page = ActiveTextPage();
  if (page == NULL) {
    return false;
  }

  switch (menu_id) {
  case ID_MENU_EDIT_UNDO:
    return page->CanUndo();

  case ID_MENU_EDIT_REDO:
    return page->CanRedo();

  case ID_MENU_EDIT_PASTE:
    // TODO
    return true;

  default:
    return true;
  }
}

bool BookFrame::GetMenuEnableState(int menu_id) const {
  if (menu_id >= ID_MENU_FILE_BEGIN && menu_id < ID_MENU_FILE_END) {
    return GetFileMenuEnableState(menu_id);
  } else if (menu_id >= ID_MENU_EDIT_BEGIN && menu_id < ID_MENU_EDIT_END) {
    return GetEditMenuEnableState(menu_id);
  }

  return true;
}

TextBook* BookFrame::ActiveTextBook() const {
  return text_books_[0];
}

TextPage* BookFrame::CreateTextPage(editor::TextBuffer* buffer,
                                    wxWindow* parent,
                                    wxWindowID id) {
  TextPage* page = new TextPage(buffer);

  page->set_style(style_);
  page->set_theme(theme_->GetTheme(THEME_TEXT_PAGE));
  page->set_binding(binding_);
  page->set_leader_key(&leader_key_);

  page->Create(parent, id, true);

  page->SetTextFont(options_->font);
  page->SetLineNrFont(options_->font);

  return page;
}

TextPage* BookFrame::TextPageByFileName(const wxFileName& fn_object) const {
  for (size_t i = 0; i < text_books_.size(); ++i) {
    std::vector<TextPage*> text_pages = BookTextPages(text_books_[i]);

    for (size_t j = 0; j < text_pages.size(); ++j) {
      if (fn_object == text_pages[j]->buffer()->file_name_object()) {
        return text_pages[j];
      }
    }
  }

  return NULL;
}

void BookFrame::RemovePage(const TextPage* page) {
  ActiveTextBook()->RemovePage(page);
}

void BookFrame::RemoveAllPages(const TextPage* except_page) {
  ActiveTextBook()->RemoveAllPages(except_page);
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

}  // namespace jil
