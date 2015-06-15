#include "app/app.h"

#include <string>
#include "boost/algorithm/string.hpp"

#if defined(__WXMSW__) && defined(_DEBUG) && defined(JIL_ENABLE_VLD)
#pragma message ("< include vld.h >")
#include "vld/vld.h"
#pragma comment(lib, "vld")
#endif  // __WXMSW__

#include "wx/cmdline.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/image.h"
#include "wx/intl.h"
#include "wx/log.h"
#include "wx/msgdlg.h"
#include "wx/preferences.h"
#include "wx/snglinst.h"
#include "wx/stdpaths.h"
#include "wx/sysopt.h"

#include "base/string_util.h"

#include "ui/color.h"

#include "editor/file_io.h"
#include "editor/ft_plugin.h"
#include "editor/style.h"
#include "editor/text_buffer.h"
#include "editor/text_line.h"
#include "editor/text_window.h"
#include "editor/util.h"

#include "app/binding_config.h"
#include "app/book_ctrl.h"
#include "app/book_frame.h"
#include "app/compile_config.h"
#include "app/font_util.h"
#include "app/goto_dialog.h"
#include "app/i18n_strings.h"
#include "app/lex_config.h"
#include "app/option_config.h"
#include "app/pref/editor_page.h"
#include "app/pref/general_page.h"
#include "app/status_fields_config.h"
#include "app/theme_config.h"
#include "app/util.h"

#define kTrPlainText _("Plain Text")

IMPLEMENT_WXWIN_MAIN

namespace jil {

////////////////////////////////////////////////////////////////////////////////
 
static const wxChar kDotChar = wxT('.');

static const wxString kTxt = wxT("txt");
static const wxString kCfgExt = wxT(".cfg");

static const wxString kFtPluginDir = wxT("ftplugin");
static const wxString kThemeDir = wxT("theme");

static const wxString kLexFile = wxT("lex.cfg");
static const wxString kOptionsFile = wxT("options.cfg");
static const wxString kIndentFile = wxT("indent.lua");
static const wxString kStatusFieldsFile = wxT("status_fields.cfg");
static const wxString kSessionFile = wxT("session.cfg");
static const wxString kBindingFile = wxT("binding.cfg");
static const wxString kFileTypesFile = wxT("file_types.cfg");

static const wxString kSpaceStr = wxT(" ");

// For Unix, this name is used to create the domain socket.
static const wxString kIpcService = wxT("jil_ipc_service");
static const wxString kIpcTopic = wxT("jil_ipc_topic");

////////////////////////////////////////////////////////////////////////////////
// Text functions.

void Goto(editor::TextWindow* tw) {
  GotoDialog goto_dialog(tw, ID_GOTO_DIALOG);
  goto_dialog.CenterOnParent();
  if (goto_dialog.ShowModal() == wxID_OK) {
    int line = goto_dialog.line();
    if (line != 0) {
      tw->Goto(line);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Void functions.

#define GET_BOOK_FRAME_OR_RETURN\
  BookFrame* book_frame = wxDynamicCast(wxGetApp().GetTopWindow(), BookFrame);\
  if (book_frame == NULL) {\
    return;\
  }

void FileNew() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileNew();
}

#if JIL_MULTIPLE_WINDOW
void FileNewWindow() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileNewWindow();
}
#endif  // JIL_MULTIPLE_WINDOW

void FileOpen() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileOpen();
}

void FileClose() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileClose();
}

void FileCloseAll() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileCloseAll();
}

void FileCloseAllButThis() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileCloseAllButThis();
}

void FileSave() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileSave();
}

void FileSaveAs() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileSaveAs();
}

void FileSaveAll() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileSaveAll();
}

void FileCopyPath() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileCopyPath();
}

void FileOpenFolder() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FileOpenFolder();
}

void NextPage() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->SwitchToNextPage();
}

void PrevPage() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->SwitchToPrevPage();
}

void NextStackPage() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->SwitchToNextStackPage();
}

void PrevStackPage() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->SwitchToPrevStackPage();
}

void ShowFind() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->ShowFind();
}

void ShowReplace() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->ShowReplace();
}

void Wrap() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->Wrap();
}

void ShowNumber() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->ShowNumber();
}

void ShowSpace() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->ShowSpace();
}

void FullScreen() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FullScreen();
}

////////////////////////////////////////////////////////////////////////////////

#if JIL_SINGLE_INSTANCE

class Connection : public wxConnection {
 public:
  virtual bool OnExec(const wxString& topic, const wxString& data) override {
    BookFrame* book_frame = wxDynamicCast(wxGetApp().GetTopWindow(), BookFrame);
    if (book_frame == NULL) {
      return false;
    }

    // Raise the book frame.
    if (book_frame->IsIconized()) {
      book_frame->Iconize(false);
    }
    book_frame->Raise();

    // Open the files.
    if (!data.IsEmpty()) {
      book_frame->OpenFiles(wxSplit(data, wxT(';')), false);
    }

    return true;
  }
};

// The 1st instance will create a server for IPC.
class Server : public wxServer {
 public:
  virtual wxConnectionBase* OnAcceptConnection(const wxString& topic) override {
    if (topic != kIpcTopic) {
      return NULL;
    }

    // Check that there are no modal dialogs active.
    wxWindowList::Node* node = wxTopLevelWindows.GetFirst();
    while (node != NULL) {
      wxDialog* dialog = wxDynamicCast(node->GetData(), wxDialog);
      if (dialog != NULL && dialog->IsModal()) {
        return NULL;
      }
      node = node->GetNext();
    }

    return new Connection;
  }
};

// The 2nd instance communicates with the 1st instance as a client.
class Client : public wxClient {
 public:
  virtual wxConnectionBase* OnMakeConnection() override {
    return new Connection;
  }
};

#endif  // JIL_SINGLE_INSTANCE

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_APP_NO_MAIN(App)

App::App()
    : instance_checker_(NULL)
    , server_(NULL)
    , log_file_(NULL)
    , style_(new editor::Style)
    , binding_(new editor::Binding) {
}

// If OnInit() returns false, OnExit() won't be called.
App::~App() {
  editor::ClearContainer(&file_types_);
  editor::ClearContainer(&ft_plugins_);

  wxDELETE(binding_);
  wxDELETE(style_);

#if JIL_SINGLE_INSTANCE
  wxDELETE(server_);
  wxDELETE(instance_checker_);
#endif  // JIL_SINGLE_INSTANCE
}

bool App::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }

#if JIL_SINGLE_INSTANCE
  // Single instance check and the communication between two instances.

  const wxString kInstanceName = wxString(kAppName) + wxT("-") + wxGetUserId();
  instance_checker_ = new wxSingleInstanceChecker(kInstanceName);

  if (!instance_checker_->IsAnotherRunning()) {
    server_ = new Server;
    if (!server_->Create(kIpcService)) {
      wxDELETE(server_);
      ErrorMsg(_("Failed to create an IPC service!"));
      return false;
    }
  } else {
    // Another instance is running.
    // Connect to it and send it any file name before exiting.
    Client* client = new Client;

    wxConnectionBase* connection = client->MakeConnection(wxT("localhost"), kIpcService, kIpcTopic);
    if (connection != NULL) {
      // Ask the other instance to open a file or raise itself.
      connection->Execute(wxJoin(cmdline_files_, wxT(';')));
      connection->Disconnect();
      delete connection;
    } else {
      ErrorMsg(_("Failed to connect to the existing instance. Any modal dialogs opened?"));  // NOLINT
    }

    delete client;
    return false;
  }
#endif  // JIL_SINGLE_INSTANCE

  SetAppName(kAppName);
  SetAppDisplayName(kAppDisplayName);

  wxImage::AddHandler(new wxPNGHandler);

#if 0  // TODO
  #ifdef __WXMSW__
  // Switched off the colour remapping and use a transparent background
  // for toolbar bitmaps.
  if (wxTheApp->GetComCtl32Version() >= 600 && ::wxDisplayDepth() >= 32) {
    wxSystemOptions::SetOption(_T("msw.remap"), 2);
  }
  #endif  // __WXMSW__
#endif  // 0

  // Make sure the user data dir exists.
  wxString user_data_dir = UserDataDir();
  if (!wxDir::Exists(user_data_dir)) {
    wxMkdir(user_data_dir, 777);
  }

#ifdef __WXMSW__
#  ifndef __WXDEBUG__

  // Set log target to file.
  wxString log_file_path = UserDataFile(wxT("log.txt"));
  log_file_ = wxFopen(log_file_path, "w+");
  wxLog* default_log = wxLog::SetActiveTarget(new wxLogStderr(log_file_));
  delete default_log;

#  endif  // __WXDEBUG__
#endif  // __WXMSW__

  LoadStatusFields();

  LoadOptions();

  if (!LoadTheme()) {
    return false;
  }

  InitCommands();

  LoadBinding();

  LoadFileTypes();

  // Load session.
  session_.Load(user_data_dir + kSessionFile);

  // Create book frame.
  BookFrame* book_frame = new BookFrame(&options_, &session_);
  book_frame->set_theme(theme_);
  book_frame->set_style(style_);
  book_frame->set_binding(binding_);

  if (!book_frame->Create(NULL, wxID_ANY, kAppDisplayName)) {
    wxLogError(wxT("Failed to create book frame!"));
    return false;
  }

  SetTopWindow(book_frame);
  book_frame->Show();

  if (options_.restore_files) {
    book_frame->RestoreOpenedFiles();
  }

  // Open the files specified as command line arguments.
  if (!cmdline_files_.empty()) {
    book_frame->OpenFiles(cmdline_files_, false);
    cmdline_files_.clear();
  }

  return true;
}

int App::OnExit() {
  session_.Save(UserDataFile(kSessionFile));

  delete wxLog::SetActiveTarget(NULL);
  if (log_file_ != NULL) {
    fclose(log_file_);
  }

  return wxApp::OnExit();
}

const editor::FileType& App::FileTypeFromExt(const wxString& ext) const {
  ExtFtMap::const_iterator it = ext_ft_map_.find(ext);
  if (it != ext_ft_map_.end()) {
    return *(it->second);
  }

  // Unsupported ext, use Plain Text file type.
  it = ext_ft_map_.find(kTxt);
  assert(it != ext_ft_map_.end());
  return *(it->second);
}

editor::FtPlugin* App::GetFtPlugin(const editor::FileType& ft) {
  for (size_t i = 0; i < ft_plugins_.size(); ++i) {
    if (ft.id == ft_plugins_[i]->id()) {
      return ft_plugins_[i];
    }
  }

  editor::FtPlugin* ft_plugin = new editor::FtPlugin(ft);

  wxString ftplugin_dir = ResourceDir(kFtPluginDir, ft_plugin->id());
  wxString ftplugin_user_dir = UserDataDir(kFtPluginDir, ft_plugin->id());

  // Lex
  LoadLexFile(ftplugin_dir + kLexFile, ft_plugin);

  // Options

  // Copy global options.
  editor::Options ft_editor_options = editor_options_;

  // Overwrite with the file type specific values.
  Config config;
  if (config.Load(ftplugin_dir + kOptionsFile)) {
    Setting editor_setting = config.Root().Get("editor");
    if (editor_setting) {
      ParseEditorOptions(editor_setting, &ft_editor_options);
    }
  }

  // Some extra view options.
  ft_editor_options.view.line_padding = options_.line_padding;
  ft_editor_options.view.min_font_size = kMinFontSize;
  ft_editor_options.view.max_font_size = kMaxFontSize;

  ft_plugin->set_options(ft_editor_options);

  ft_plugins_.push_back(ft_plugin);
  return ft_plugin;
}

void App::ShowPreferencesEditor(wxWindow* parent) {
  if (!pref_editor_) {
    pref_editor_.reset(new wxPreferencesEditor(kTrOptions));
    pref_editor_->AddPage(new pref::GeneralPage(&options_));
    pref_editor_->AddPage(new pref::EditorPage());
  }

  pref_editor_->Show(parent);
}

void App::DismissPreferencesEditor() {
  if (pref_editor_) {
    pref_editor_->Dismiss();
  }
}

// Command line parsing.

#if wxUSE_CMDLINE_PARSER

void App::OnInitCmdLine(wxCmdLineParser& parser) {
  wxApp::OnInitCmdLine(parser);
  parser.AddParam(_("file"),
                  wxCMD_LINE_VAL_STRING,
                  wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL);
}

bool App::OnCmdLineParsed(wxCmdLineParser& parser) {
  wxApp::OnCmdLineParsed(parser);

  size_t param_count = parser.GetParamCount();
  for (size_t i = 0; i < param_count; ++i) {
    cmdline_files_.push_back(parser.GetParam(i));
  }

  return true;
}

bool App::OnCmdLineHelp(wxCmdLineParser& parser) {
  // Print usage and exit (by returning false).
  parser.Usage();
  return false;
}

bool App::OnCmdLineError(wxCmdLineParser& parser) {
  // Incorrect command line options were given. Return false to abort.
  return false;
}

#endif  // wxUSE_CMDLINE_PARSER

const wxString& App::UserDataDir() const {
  static const wxString user_data_dir =
      wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH;
  return user_data_dir;
}

wxString App::UserDataDir(const wxString& dir) const {
  return UserDataDir() + dir + wxFILE_SEP_PATH;
}

wxString App::UserDataDir(const wxString& dir, const wxString& dir2) const {
  return UserDataDir(dir) + dir2 + wxFILE_SEP_PATH;
}

wxString App::UserDataFile(const wxString& file) const {
  return UserDataDir() + file;
}

wxString App::UserDataFile(const wxString& dir, const wxString& file) const {
  return UserDataDir(dir) + file;
}

wxString App::ResourceDir(const wxString& dir) const {
  return path::ResourceDir() + dir + wxFILE_SEP_PATH;
}

wxString App::ResourceDir(const wxString& dir, const wxString& dir2) const {
  return ResourceDir(dir) + dir2 + wxFILE_SEP_PATH;
}

wxString App::ResourceFile(const wxString& file) const {
  return path::ResourceDir() + file;
}

wxString App::ResourceFile(const wxString& dir, const wxString& file) const {
  return path::ResourceDir() + dir + wxFILE_SEP_PATH + file;
}

void App::LoadStatusFields() {
  wxString status_fields_file = UserDataFile(kStatusFieldsFile);
  if (!wxFileName::FileExists(status_fields_file)) {
    status_fields_file = ResourceFile(kStatusFieldsFile);
  }

  Config config;
  if (!config.Load(status_fields_file)) {
    ErrorMsg(kTrFailedToLoad + kSpaceStr + status_fields_file);
    return;
  }

  Setting list_setting = config.Root()[0];
  if (!list_setting || list_setting.type() != Setting::kList) {
    return;
  }

  ParseStatusFields(list_setting, &status_fields_);
}

// Load user option file firstly.
// If user option file doesn't exist, load the default one.
void App::LoadOptions() {
  wxString options_file = UserDataFile(kOptionsFile);
  if (!wxFileName::FileExists(options_file)) {
    wxLogInfo(wxT("No user options config file."));
    options_file = ResourceFile(kOptionsFile);
  }

  Config config;
  if (!config.Load(options_file)) {
    ErrorMsg(kTrFailedToLoad + kSpaceStr + options_file);
    return;
  }

  Setting app_setting = config.Root().Get("app");
  if (app_setting) {
    ParseAppOptions(app_setting, &options_);
  }

  Setting editor_setting = config.Root().Get("editor");
  if (editor_setting) {
    ParseEditorOptions(editor_setting, &editor_options_);
  }
}

bool App::LoadTheme() {
  // List installed themes.
  wxDir theme_dir(ResourceDir(kThemeDir));
  if (theme_dir.IsOpened()) {
    wxString theme_name;
    bool cont = theme_dir.GetFirst(&theme_name, wxT("*.cfg"));
    while (cont) {
      theme_names_.push_back(theme_name.BeforeLast(kDotChar));
      cont = theme_dir.GetNext(&theme_name);
    }
  }

  if (theme_names_.empty()) {
    ErrorMsg(_("No theme is installed!"));
    return false;
  }

  theme_.reset(new editor::Theme(THEME_COUNT));

  wxString theme_name = options_.theme;
  if (theme_name.empty()) {
    theme_name = theme_names_.front();
  } else if (std::find(theme_names_.begin(), theme_names_.end(), theme_name) == theme_names_.end()) {
    theme_name = theme_names_.front();
  }

  wxString theme_file = ResourceFile(kThemeDir, theme_name + kCfgExt);

  if (!LoadThemeFile(theme_file, theme_, style_)) {
    ErrorMsg(kTrFailedToLoad + kSpaceStr + theme_file);
    return false;
  }

  return true;
}

void App::InitCommands() {
  using namespace editor;  // NOLINT

  //----------------------------------------------------------------------------

  AddTextCmd("new_line_break", NewLineBreak, true, 0);
  AddTextCmd("new_line_below", NewLineBelow, true, 0);
  AddTextCmd("new_line_above", NewLineAbove, true, 0);

  AddTextCmd("undo", Undo, true, ID_MENU_EDIT_UNDO);
  AddTextCmd("redo", Redo, true, ID_MENU_EDIT_REDO);

  AddTextCmd("cut", Cut, true, ID_MENU_EDIT_CUT);
  AddTextCmd("copy", Copy, false, ID_MENU_EDIT_COPY);
  AddTextCmd("paste", Paste, true, ID_MENU_EDIT_PASTE);

  AddTextCmd("auto_indent", AutoIndent, true, ID_MENU_EDIT_AUTO_INDENT);
  AddTextCmd("increase_indent",
             IncreaseIndent,
             true,
             ID_MENU_EDIT_INCREASE_INDENT);
  AddTextCmd("decrease_indent",
             DecreaseIndent,
             true,
             ID_MENU_EDIT_DECREASE_INDENT);

  AddTextCmd("comment", Comment, true, ID_MENU_EDIT_COMMENT);
  AddTextCmd("uncomment", Uncomment, true, ID_MENU_EDIT_UNCOMMENT);
  AddTextCmd("goto", Goto, false, ID_MENU_EDIT_GO_TO);

  //----------------------------------------------------------------------------

  AddMoveTextCmd(kChar, kPrev, 0);
  AddMoveTextCmd(kChar, kNext, 0);
  AddMoveTextCmd(kSelected, kBegin, 0);
  AddMoveTextCmd(kSelected, kEnd, 0);
  AddMoveTextCmd(kWord, kPrev, 0);
  AddMoveTextCmd(kWord, kNext, 0);
  AddMoveTextCmd(kLine, kPrev, 0);
  AddMoveTextCmd(kLine, kNext, 0);
  AddMoveTextCmd(kLine, kBegin, 0);
  AddMoveTextCmd(kLine, kEnd, 0);
  AddMoveTextCmd(kBuffer, kBegin, 0);
  AddMoveTextCmd(kBuffer, kEnd, 0);

  AddDeleteTextCmd(kChar, kPrev, 0);
  AddDeleteTextCmd(kChar, kNext, 0);
  AddDeleteTextCmd(kWord, kPrev, 0);
  AddDeleteTextCmd(kWord, kNext, 0);
  AddDeleteTextCmd(kLine, kBegin, 0);
  AddDeleteTextCmd(kLine, kEnd, 0);
  AddDeleteTextCmd(kLine, kWhole, 0);
  AddDeleteTextCmd(kSelected, kWhole, 0);

  AddScrollTextCmd(kLine, kPrev, 0);
  AddScrollTextCmd(kLine, kNext, 0);
  AddScrollTextCmd(kHalfPage, kPrev, 0);
  AddScrollTextCmd(kHalfPage, kNext, 0);
  AddScrollTextCmd(kPage, kPrev, 0);
  AddScrollTextCmd(kPage, kNext, 0);

  AddSelectTextCmd(kChar, kPrev, 0);
  AddSelectTextCmd(kChar, kNext, 0);
  AddSelectTextCmd(kWord, kPrev, 0);
  AddSelectTextCmd(kWord, kNext, 0);
  AddSelectTextCmd(kLine, kBegin, 0);
  AddSelectTextCmd(kLine, kEnd, 0);
  AddSelectTextCmd(kLine, kPrev, 0);
  AddSelectTextCmd(kLine, kNext, 0);
  AddSelectTextCmd(kBuffer, kBegin, 0);
  AddSelectTextCmd(kBuffer, kEnd, 0);
  AddSelectTextCmd(kBuffer, kWhole, 0);
  AddSelectTextCmd(kSelected, kWhole, 0);

  //----------------------------------------------------------------------------

  AddVoidCmd("new", FileNew, ID_MENU_FILE_NEW);
#if JIL_MULTIPLE_WINDOW
  AddVoidCmd("new_window", FileNewWindow, ID_MENU_FILE_NEW_WINDOW);
#endif  // JIL_MULTIPLE_WINDOW
  AddVoidCmd("open", FileOpen, ID_MENU_FILE_OPEN);
  AddVoidCmd("close", FileClose, ID_MENU_FILE_CLOSE);
  AddVoidCmd("close_all", FileCloseAll, ID_MENU_FILE_CLOSE_ALL);
  AddVoidCmd("close_all_but_this", FileCloseAllButThis, ID_MENU_FILE_CLOSE_ALL_BUT_THIS);

  AddVoidCmd("save", FileSave, ID_MENU_FILE_SAVE);
  AddVoidCmd("save_as", FileSaveAs, ID_MENU_FILE_SAVE_AS);
  AddVoidCmd("save_all", FileSaveAll, ID_MENU_FILE_SAVE_ALL);
  AddVoidCmd("copy_path", FileCopyPath, ID_MENU_FILE_COPY_PATH);
  AddVoidCmd("open_folder", FileOpenFolder, ID_MENU_FILE_OPEN_FOLDER);

  AddVoidCmd("show_find", ShowFind, ID_MENU_EDIT_FIND);
  AddVoidCmd("show_replace", ShowReplace, ID_MENU_EDIT_REPLACE);

  AddVoidCmd("next_page", NextPage, 0);
  AddVoidCmd("prev_page", PrevPage, 0);
  AddVoidCmd("next_stack_page", NextStackPage, 0);
  AddVoidCmd("prev_stack_page", PrevStackPage, 0);

  AddVoidCmd("wrap", Wrap, ID_MENU_VIEW_WRAP);
  AddVoidCmd("show_number", ShowNumber, ID_MENU_VIEW_SHOW_NUMBER);
  AddVoidCmd("show_space", ShowSpace, ID_MENU_VIEW_SHOW_SPACE);
  AddVoidCmd("full_screen", FullScreen, ID_MENU_VIEW_FULL_SCREEN);
}

void App::AddTextCmd(const char* name,
                     editor::RawTextFunc func,
                     bool change_text,
                     int menu) {
  using namespace editor;
  TextFunc* text_func = new TextFuncWrap(func, change_text);
  binding_->AddTextCmd(name, text_func, menu);
}

void App::AddMoveTextCmd(editor::TextUnit unit,
                         editor::SeekType seek,
                         int menu) {
  static const std::string kMovePrefix = "move.";
  AddSeekableTextCmd(kMovePrefix, editor::Move, unit, seek, menu, false);
}

void App::AddDeleteTextCmd(editor::TextUnit unit,
                           editor::SeekType seek,
                           int menu) {
  static const std::string kDeletePrefix = "delete.";
  AddSeekableTextCmd(kDeletePrefix, editor::Delete, unit, seek, menu, true);
}

void App::AddScrollTextCmd(editor::TextUnit unit,
                           editor::SeekType seek,
                           int menu) {
  static const std::string kScrollPrefix = "scroll.";
  AddSeekableTextCmd(kScrollPrefix, editor::Scroll, unit, seek, menu, false);
}

void App::AddSelectTextCmd(editor::TextUnit unit,
                           editor::SeekType seek,
                           int menu) {
  static const std::string kSelectPrefix = "select.";
  AddSeekableTextCmd(kSelectPrefix, editor::Select, unit, seek, menu, false);
}

void App::AddSeekableTextCmd(const std::string& name_prefix,
                             editor::RawSeekableTextFunc func,
                             editor::TextUnit unit,
                             editor::SeekType seek,
                             int menu,
                             bool change_text) {
  using namespace editor;
  TextFunc* text_func = new SeekableTextFuncWrap(func, unit, seek);
  text_func->set_change_text(change_text);

  std::string name = name_prefix + UnitName(unit) + "." + SeekName(seek);
  binding_->AddTextCmd(name, text_func, menu);
}

void App::AddVoidCmd(const char* name, editor::RawVoidFunc func, int menu) {
  binding_->AddVoidCmd(name, new editor::VoidFuncWrap(func), menu);
}

bool App::LoadBinding() {
  BindingConfig binding_config(binding_);

  // Always load default binding.
  wxString binding_file = ResourceFile(kBindingFile);
  if (!binding_config.Load(binding_file)) {
    ErrorMsg(kTrFailedToLoad + kSpaceStr + binding_file);
    return false;
  }

  if (!binding_config.Load(UserDataFile(kBindingFile))) {
    wxLogInfo(wxT("No user binding."));
  }

  binding_->BindMenus();

  return true;
}

bool App::LoadFileTypes() {
  using namespace editor;

  // Plain Text file type.
  FileType* txt_ft = new FileType(kTxt, kTrPlainText);
  file_types_.push_back(txt_ft);
  ext_ft_map_[wxEmptyString] = txt_ft;
  ext_ft_map_[kTxt] = txt_ft;

  // Load other file types from config file.
  wxString file_types_file = path::ResourceDir() + kFileTypesFile;

  Config config;
  if (!config.Load(file_types_file)) {
    ErrorMsg(kTrFailedToLoad + kSpaceStr + file_types_file);
    return false;
  }

  Setting root_setting = config.Root();

  Setting list_setting = root_setting.Get("list");
  if (!list_setting || list_setting.type() != Setting::kList) {
    return false;
  }

  int count = list_setting.size();

  for (int j = 0; j < count; ++j) {
    Setting setting = list_setting.Get(j);

    wxString id = wxString::FromAscii(setting.GetString("id"));
    wxString name = wxString::FromAscii(setting.GetString("name"));
    std::string ext = setting.GetString("ext");

    FileType* ft = new FileType(id, name);
    file_types_.push_back(ft);

    // Map ext to file type.
    std::vector<std::string> ext_array;
    boost::split(ext_array,
                 ext,
                 boost::is_any_of(","),
                 boost::token_compress_on);

    for (size_t i = 0; i < ext_array.size(); ++i) {
      wxString ext_key = wxString::FromAscii(ext_array[i].c_str());
      if (ext_ft_map_.find(ext_key) == ext_ft_map_.end()) {
        ext_ft_map_[ext_key] = ft;
      } else {
        wxLogError(wxT("Can't map ext [%s] to file type [%s]."), ext_key, ft->name);
      }
    }
  }

  return true;
}

}  // namespace jil
