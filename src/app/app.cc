#include "app/app.h"

#include <string>

#if defined(__WXMSW__) && defined(_DEBUG) && defined(JIL_ENABLE_VLD)
#pragma message ("< include vld.h >")
#include "vld/vld.h"
#pragma comment(lib, "vld")
#endif  // __WXMSW__

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

#include "wx/clipbrd.h"
#include "wx/cmdline.h"
#include "wx/debugrpt.h"
#include "wx/dir.h"
#include "wx/filename.h"
#include "wx/image.h"
#include "wx/intl.h"
#include "wx/log.h"
#include "wx/msgdlg.h"
#include "wx/snglinst.h"
#include "wx/stdpaths.h"
#include "wx/sysopt.h"

#include "base/string_util.h"

#include "ui/color.h"

#include "editor/ft_plugin.h"
#include "editor/lua_proxy.h"
#include "editor/style.h"
#include "editor/text_window.h"
#include "editor/util.h"

#include "app/binding_config.h"
#include "app/book_ctrl.h"
#include "app/book_frame.h"
#include "app/font_util.h"
#include "app/goto_dialog.h"
#include "app/i18n_strings.h"
#include "app/lex_config.h"
#include "app/option_config.h"
#include "app/status_fields_config.h"
#include "app/theme_config.h"
#include "app/util.h"

#if defined(__WXMSW__)
#include "app/resource.h"  // For icon IDs.
#endif

IMPLEMENT_WXWIN_MAIN

namespace jil {

////////////////////////////////////////////////////////////////////////////////

static const wxString kSpaceStr = wxT(" ");

static const wxString kCfgExt = wxT(".cfg");

static const wxString kFtPluginDir = wxT("ftplugin");
static const wxString kThemeDir = wxT("theme");

static const wxString kLexCfgFile = wxT("lex.cfg");
static const wxString kOptionsCfgFile = wxT("options.cfg");
static const wxString kIndentLuaFile = wxT("indent.lua");
static const wxString kSessionCfgFile = wxT("session.cfg");
static const wxString kBindingCfgFile = wxT("binding.cfg");
static const wxString kFtCfgFile = wxT("ft.cfg");
static const wxString kStatusFieldsCfgFile = wxT("status_fields.cfg");

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

void FindNext() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FindNext();
}

void FindPrev() {
  GET_BOOK_FRAME_OR_RETURN;
  book_frame->FindPrev();
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
    : log_file_(NULL)
#if JIL_SINGLE_INSTANCE
    , instance_checker_(NULL)
    , server_(NULL)
#endif  // JIL_SINGLE_INSTANCE
    , style_(new editor::Style)
    , binding_(new editor::Binding) {
  lua_state_ = luaL_newstate();
  luaL_openlibs(lua_state_);
}

// If OnInit() returns false, OnExit() won't be called.
App::~App() {
  editor::ClearContainer(&ft_plugins_);

  wxDELETE(binding_);
  wxDELETE(style_);

#if JIL_SINGLE_INSTANCE
  wxDELETE(server_);
  wxDELETE(instance_checker_);
#endif  // JIL_SINGLE_INSTANCE

  lua_close(lua_state_);
}

bool App::OnInit() {
  if (!wxApp::OnInit()) {
    return false;
  }

#if wxUSE_ON_FATAL_EXCEPTION
  // OnFatalException() will be called on fatal exceptions.
  wxHandleFatalExceptions();
#endif

#if JIL_SINGLE_INSTANCE
  if (!InitIpc()) {
    return false;
  }
#endif  // JIL_SINGLE_INSTANCE

  SetAppName(kAppName);
  SetAppDisplayName(kAppDisplayName);

  wxImage::AddHandler(new wxPNGHandler);

#if 0  // We don't have toolbars.
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
    wxMkdir(user_data_dir, 0777);
  }

  InitLogging();

  LoadStatusFields();

  LoadOptions();

  ListThemes();

  if (!LoadTheme()) {
    return false;
  }

  InitCommands();

  LoadBinding();

  LoadFileTypes();

  // Load session.
  session_.Load(user_data_dir + kSessionCfgFile);

  editor::InitLua(lua_state_);

  // Create book frame.
  BookFrame* book_frame = new BookFrame();

  book_frame->set_options(&options_);
  book_frame->set_editor_options(&editor_options_);
  book_frame->set_session(&session_);
  book_frame->set_theme(theme_);
  book_frame->set_style(style_);
  book_frame->set_binding(binding_);

  if (!book_frame->Create(NULL, wxID_ANY, kAppDisplayName)) {
    wxLogError(wxT("Failed to create book frame!"));
    return false;
  }

  LoadIcons();
  book_frame->SetIcons(icons_);

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
  // Flushes the clipboard: this means that the data which is currently on
  // clipboard will stay available even after the application exits (possibly
  // eating memory), otherwise the clipboard will be emptied on exit.
  wxTheClipboard->Flush();

  session_.Save(UserDataFile(kSessionCfgFile));

  delete wxLog::SetActiveTarget(NULL);
  if (log_file_ != NULL) {
    fclose(log_file_);
  }

  return wxApp::OnExit();
}

#if wxUSE_ON_FATAL_EXCEPTION

void App::OnFatalException() {
#if wxUSE_DEBUGREPORT
  wxDebugReportCompress debug_report;

  debug_report.SetCompressedFileDirectory(UserDataDir());

  wxString name = wxDateTime::Now().Format(_T("dbgrpt_%Y%m%d_%H%M%S"));
  debug_report.SetCompressedFileBaseName(name);

  debug_report.AddAll(wxDebugReport::Context_Exception);
  debug_report.Process();

#endif // wxUSE_DEBUGREPORT
}

#endif // wxUSE_ON_FATAL_EXCEPTION

int App::GetThemeCount() const {
  return static_cast<int>(theme_names_.size());
}

wxString App::GetTheme(int index) const {
  assert(index >= 0 && index < GetThemeCount());
  return theme_names_[index];
}

int App::GetFileTypeCount() const {
  return static_cast<int>(ft_config_.GetCount());
}

const editor::FileType* App::GetFileType(int index) const {
  assert(index >= 0 && index < GetFileTypeCount());
  return ft_config_.GetByIndex(static_cast<size_t>(index));
}

const editor::FileType& App::FileTypeFromFileName(const wxFileName& fn) const {
  return ft_config_.GetByFileName(fn);
}

editor::FtPlugin* App::GetFtPlugin(const editor::FileType& ft) {
  for (size_t i = 0; i < ft_plugins_.size(); ++i) {
    if (ft.id == ft_plugins_[i]->id()) {
      return ft_plugins_[i];
    }
  }

  editor::FtPlugin* ft_plugin = new editor::FtPlugin(ft, lua_state_);

  wxString ft_plugin_dir = ResourceDir(kFtPluginDir, ft_plugin->id());
  wxString ft_plugin_user_dir = UserDataDir(kFtPluginDir, ft_plugin->id());

  //----------------------------------------------------------------------------
  // Load lex

  wxString ft_lex_cfg_file = ft_plugin_user_dir + kLexCfgFile;
  if (!wxFileName::FileExists(ft_lex_cfg_file)) {
    ft_lex_cfg_file = ft_plugin_dir + kLexCfgFile;
  }

  LoadLexFile(ft_lex_cfg_file, ft_plugin);

  //----------------------------------------------------------------------------
  // Load editor options

  // Copy global editor options.
  editor::Options ft_editor_options = editor_options_;

  wxString ft_options_cfg_file = ft_plugin_user_dir + kOptionsCfgFile;
  if (!wxFileName::FileExists(ft_options_cfg_file)) {
    ft_options_cfg_file = ft_plugin_dir + kOptionsCfgFile;
  }

  // Overwrite with the file type specific values.
  LoadEditorOptionsFile(ft_options_cfg_file, &ft_editor_options);

  ft_plugin->set_options(ft_editor_options);

  //----------------------------------------------------------------------------
  // Indent

  WorkingDirSwitcher wd_switcher(ft_plugin_dir);

  // Load indent function from Lua script.
  std::string lua_error;
  if (editor::LoadLuaFile(lua_state_, kIndentLuaFile, &lua_error)) {
    std::string ns = ft.id.ToStdString();
    luabridge::LuaRef indent_func = editor::GetLuaValue(lua_state_, ns.c_str(), "indent");
    ft_plugin->set_indent_func(indent_func);
  } else {
    if (!lua_error.empty()) {
      // Save the error to file type plugin.
      int ln = 0;
      std::string msg;
      if (editor::ParseLuaError(lua_error, &ln, &msg)) {
        ft_plugin->AddLuaIndentError(ln, msg);
      }
    }
  }

  ft_plugins_.push_back(ft_plugin);
  return ft_plugin;
}

editor::FtPlugin* App::GetFtPluginByFileName(const wxFileName& fn) {
  return GetFtPlugin(ft_config_.GetByFileName(fn));
}

bool App::SaveUserGlobalOptions() {
  wxString options_file = UserDataFile(kOptionsCfgFile);
  return SaveGlobalOptionsFile(options_file, options_);
}

bool App::SaveUserEditorOptions(const wxString& ft_id, const editor::Options& options) {
  wxString ft_options_dir = UserDataDir(kFtPluginDir, ft_id);

  if (!editor::MakeDirFully(ft_options_dir)) {
    wxLogError(wxT("Failed to create directory: %s"), ft_options_dir);
    return false;
  }

  return SaveEditorOptionsFile(ft_options_dir + kOptionsCfgFile, options);
}

bool App::ReloadTheme(const wxString& theme_name) {
  wxString theme_file = ResourceFile(kThemeDir, theme_name);

  if (!LoadThemeFile(theme_file, options_.icon_resolution, theme_, style_)) {
    ShowError(wxString::Format(kTrCfgFileLoadFail, theme_file));
    return false;
  }

  return true;
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
  static const wxString user_data_dir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH;
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

wxString App::UserDataFile(const wxString& dir, const wxString& dir2, const wxString& file) const {
  return UserDataDir(dir, dir2) + file;
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
  return ResourceDir(dir) + file;
}

wxString App::ResourceFile(const wxString& dir, const wxString& dir2, const wxString& file) const {
  return ResourceDir(dir, dir2) + file;
}

bool App::InitIpc() {
  const wxString kInstanceName = wxString(kAppName) + wxT("-") + wxGetUserId();
  instance_checker_ = new wxSingleInstanceChecker(kInstanceName);

  if (!instance_checker_->IsAnotherRunning()) {
    server_ = new Server;
    if (!server_->Create(kIpcService)) {
      wxDELETE(server_);
      ShowError(_("Cannot create IPC service!"));
      return false;
    }
    return true;
  }

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
    ShowError(_("Cannot connect to the existing instance. Any modal dialogs opened?"));
  }

  delete client;
  return false;
}

void App::InitLogging() {
  // Set log target to file.
  wxString log_file_path = UserDataFile(wxT("Jil.log"));
  log_file_ = wxFopen(log_file_path, "w+");
  wxLog* default_log = wxLog::SetActiveTarget(new wxLogStderr(log_file_));
  delete default_log;
}

void App::LoadStatusFields() {
  wxString status_fields_cfg_file = UserDataFile(kStatusFieldsCfgFile);
  if (!wxFileName::FileExists(status_fields_cfg_file)) {
    status_fields_cfg_file = ResourceFile(kStatusFieldsCfgFile);
  }

  Config config;
  if (!config.Load(status_fields_cfg_file)) {
    ShowError(wxString::Format(kTrCfgFileLoadFail, status_fields_cfg_file));
    return;
  }

  Setting list_setting = config.Root()[0];
  if (!list_setting || list_setting.type() != Setting::kList) {
    return;
  }

  ParseStatusFields(list_setting, &status_fields_);
}

// If new options are added, increase the version number.
// If version number is not consistent between resource file and user file,
// always load resource file firstly.
void App::LoadOptions() {
  wxString global_options_file = UserDataFile(kOptionsCfgFile);
  if (!wxFileName::FileExists(global_options_file)) {
    wxLogInfo(wxT("No user options config file."));
    global_options_file = ResourceFile(kOptionsCfgFile);
  }

  if (!LoadGlobalOptionsFile(global_options_file, &options_)) {
    ShowError(wxString::Format(kTrCfgFileLoadFail, global_options_file));
  }

  wxString editor_options_file = ResourceFile(kFtPluginDir, wxT("__default"), kOptionsCfgFile);
  LoadEditorOptionsFile(editor_options_file, &editor_options_);
}

void App::ListThemes() {
  theme_names_.clear();

  wxDir dir(ResourceDir(kThemeDir));

  if (dir.IsOpened()) {
    wxString name;
    bool cont = dir.GetFirst(&name, wxEmptyString, wxDIR_DIRS);
    while (cont) {
      theme_names_.push_back(name);
      cont = dir.GetNext(&name);
    }
  }
}

bool App::LoadTheme() {
  if (theme_names_.empty()) {
    ShowError(_("Cannot find any theme installed!"));
    return false;
  }

  theme_.reset(new editor::Theme(THEME_COUNT));

  wxString theme_name = options_.theme;
  if (theme_name.empty()) {
    theme_name = theme_names_.front();
    options_.theme = theme_name;
    wxLogWarning(wxT("The theme is not specified, '%s' will be used."), theme_name);
  } else if (std::find(theme_names_.begin(), theme_names_.end(), theme_name) == theme_names_.end()) {
    wxLogWarning(wxT("The specified theme '%s' doesn't exist, '%s' will be used instead."),
                 theme_name,
                 theme_names_.front());
    theme_name = theme_names_.front();
    options_.theme = theme_name;
  }

  wxString theme_file = ResourceFile(kThemeDir, theme_name);

  if (!LoadThemeFile(theme_file, options_.icon_resolution, theme_, style_)) {
    ShowError(wxString::Format(kTrCfgFileLoadFail, theme_file));
    return false;
  }

  return true;
}

void App::InitCommands() {
  using namespace editor;

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
  AddTextCmd("increase_indent", IncreaseIndent, true, ID_MENU_EDIT_INCREASE_INDENT);
  AddTextCmd("decrease_indent", DecreaseIndent, true, ID_MENU_EDIT_DECREASE_INDENT);

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
  AddVoidCmd("open", FileOpen, ID_MENU_FILE_OPEN);
  AddVoidCmd("close", FileClose, ID_MENU_FILE_CLOSE);
  AddVoidCmd("close_all", FileCloseAll, ID_MENU_FILE_CLOSE_ALL);
  AddVoidCmd("close_all_but_this", FileCloseAllButThis, ID_MENU_FILE_CLOSE_ALL_BUT_THIS);

  AddVoidCmd("save", FileSave, ID_MENU_FILE_SAVE);
  AddVoidCmd("save_as", FileSaveAs, ID_MENU_FILE_SAVE_AS);
  AddVoidCmd("save_all", FileSaveAll, ID_MENU_FILE_SAVE_ALL);
  AddVoidCmd("copy_path", FileCopyPath, ID_MENU_FILE_COPY_PATH);
  AddVoidCmd("open_folder", FileOpenFolder, ID_MENU_FILE_OPEN_FOLDER);
  AddVoidCmd("next_stack_page", NextStackPage, ID_MENU_FILE_NEXT_STACK_PAGE);
  AddVoidCmd("prev_stack_page", PrevStackPage, ID_MENU_FILE_PREV_STACK_PAGE);

  AddVoidCmd("show_find", ShowFind, ID_MENU_EDIT_FIND);
  AddVoidCmd("show_replace", ShowReplace, ID_MENU_EDIT_REPLACE);

  AddVoidCmd("find_next", FindNext, ID_MENU_EDIT_FIND_NEXT);
  AddVoidCmd("find_prev", FindPrev, ID_MENU_EDIT_FIND_PREV);

  AddVoidCmd("wrap", Wrap, ID_MENU_VIEW_WRAP);
  AddVoidCmd("show_number", ShowNumber, ID_MENU_VIEW_SHOW_NUMBER);
  AddVoidCmd("show_space", ShowSpace, ID_MENU_VIEW_SHOW_SPACE);
  AddVoidCmd("full_screen", FullScreen, ID_MENU_VIEW_FULL_SCREEN);
}

void App::AddTextCmd(const char* name, editor::RawTextFunc func, bool change_text, int menu) {
  using namespace editor;
  TextFunc* text_func = new TextFuncWrap(func, change_text);
  binding_->AddTextCmd(name, text_func, menu);
}

void App::AddMoveTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu) {
  static const std::string kMovePrefix = "move.";
  AddSeekableTextCmd(kMovePrefix, editor::Move, unit, seek, menu, false);
}

void App::AddDeleteTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu) {
  static const std::string kDeletePrefix = "delete.";
  AddSeekableTextCmd(kDeletePrefix, editor::Delete, unit, seek, menu, true);
}

void App::AddScrollTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu) {
  static const std::string kScrollPrefix = "scroll.";
  AddSeekableTextCmd(kScrollPrefix, editor::Scroll, unit, seek, menu, false);
}

void App::AddSelectTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu) {
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
  wxString binding_cfg_file = ResourceFile(kBindingCfgFile);
  if (!binding_config.Load(binding_cfg_file)) {
    ShowError(wxString::Format(kTrCfgFileLoadFail, binding_cfg_file));
    return false;
  }

  if (!binding_config.Load(UserDataFile(kBindingCfgFile))) {
    wxLogInfo(wxT("No user binding."));
  }

  binding_->BindMenus();

  return true;
}

bool App::LoadFileTypes() {
  // Load other file types from config file.
  wxString ft_cfg_file = path::ResourceDir() + kFtCfgFile;

  if (!ft_config_.Load(ft_cfg_file)) {
    ShowError(wxString::Format(kTrCfgFileLoadFail, ft_cfg_file));
    return false;
  }

  return true;
}

// TODO: OSX and GTK.
void App::LoadIcons() {

#if defined(__WXMSW__)

  // NOTE:
  // wxWidgets seems not providing a function to get embedded icon resources
  // by icon ID. Windows API is used instead.

  HINSTANCE hinstance = ::GetModuleHandle(NULL);

  // NOTE: Only 16 and 32 are needed.

  HICON hicon16 = ::LoadIcon(hinstance, MAKEINTRESOURCE(IDI_EDITOR_16));
  if (hicon16 != NULL) {
    wxIcon icon16;
    if (icon16.CreateFromHICON(hicon16)) {
      icons_.AddIcon(icon16);
    }
  }

  HICON hicon32 = ::LoadIcon(hinstance, MAKEINTRESOURCE(IDI_EDITOR_32));
  if (hicon32 != NULL) {
    wxIcon icon32;
    if (icon32.CreateFromHICON(hicon32)) {
      icons_.AddIcon(icon32);
    }
  }
#endif

}

}  // namespace jil
