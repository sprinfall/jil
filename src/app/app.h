#ifndef JIL_APP_H_
#define JIL_APP_H_
#pragma once

#include <list>
#include <map>
#include <vector>

#include "app/compile_config.h"

#include "wx/app.h"
#include "wx/arrstr.h"
#include "wx/iconbndl.h"

#if JIL_SINGLE_INSTANCE
#  include "wx/ipc.h"  // wxServer
#endif

#include "editor/defs.h"
#include "editor/option.h"
#include "editor/text_func.h"
#include "editor/theme.h"

#include "app/config.h"
#include "app/defs.h"
#include "app/ft_config.h"
#include "app/option.h"
#include "app/session.h"
#include "app/status_bar.h"

struct lua_State;

namespace luabridge {
class LuaRef;
}

class wxFileName;

#if JIL_SINGLE_INSTANCE
class wxSingleInstanceChecker;
#endif

namespace jil {

namespace editor {
class Binding;
class FtPlugin;
class Style;
}  // namespace editor

class BookFrame;
class Session;

class App : public wxApp {
public:
  App();
  virtual ~App();

  virtual bool OnInit() override;
  virtual int OnExit() override;

  const Options& options() const {
    return options_;
  }

  int GetThemeCount() const;
  wxString GetTheme(int index) const;

  int GetFileTypeCount() const;
  const editor::FileType* GetFileType(int index) const;

  const editor::FileType& FileTypeFromFileName(const wxFileName& fn) const;

  // Get ft plugin object.
  // If the ft plugin doesn't exist, it will be created.
  editor::FtPlugin* GetFtPlugin(const editor::FileType& ft);

  // Get ft plugin object by file name.
  editor::FtPlugin* GetFtPluginByFileName(const wxFileName& fn);

  const std::vector<StatusBar::FieldInfo>& status_fields() const {
    return status_fields_;
  }

  // Save global options to <user_data_dir>/options.cfg
  bool SaveUserGlobalOptions();

  // Save editor options to <user_data_dir>/ftplugin/<ft_id>/options.cfg
  bool SaveUserEditorOptions(const wxString& ft_id, const editor::Options& options);

  bool ReloadTheme(const wxString& theme_name);

  const wxIconBundle& icons() const {
    return icons_;
  }

protected:
#if wxUSE_CMDLINE_PARSER
  // Called from OnInit() to add all supported options to the given parser.
  virtual void OnInitCmdLine(wxCmdLineParser& parser) override;

  // Called after successfully parsing the command line, return true
  // to continue and false to exit.
  virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

  // Called if "--help" option was specified, return true to continue
  // and false to exit.
  virtual bool OnCmdLineHelp(wxCmdLineParser& parser) override;

  // Called if incorrect command line options were given, return
  // false to abort and true to continue.
  virtual bool OnCmdLineError(wxCmdLineParser& parser) override;
#else
#error "Please enable wxUSE_CMDLINE_PARSER!"
#endif  // wxUSE_CMDLINE_PARSER

private:
  // Windows: C:\Users\username\AppData\Roaming\appname\ (Win7)
  // Mac: ~/Library/Application Support/appname/
  // Unix: ~/.appname/
  const wxString& UserDataDir() const;

  wxString UserDataDir(const wxString& dir) const;
  wxString UserDataDir(const wxString& dir, const wxString& dir2) const;

  wxString UserDataFile(const wxString& file) const;
  wxString UserDataFile(const wxString& dir, const wxString& file) const;
  wxString UserDataFile(const wxString& dir, const wxString& dir2, const wxString& file) const;

  wxString ResourceDir(const wxString& dir) const;
  wxString ResourceDir(const wxString& dir, const wxString& dir2) const;

  wxString ResourceFile(const wxString& file) const;
  wxString ResourceFile(const wxString& dir, const wxString& file) const;
  wxString ResourceFile(const wxString& dir, const wxString& dir2, const wxString& file) const;

#if JIL_SINGLE_INSTANCE
  bool InitIpc();
#endif

  void LoadStatusFields();

  void LoadOptions();

  // List all installed themes.
  void ListThemes();

  bool LoadTheme();

  // Init text and void commands.
  void InitCommands();

  void AddTextCmd(const char* name, editor::RawTextFunc func, bool change_text, int menu);

  void AddMoveTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu);
  void AddDeleteTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu);
  void AddScrollTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu);
  void AddSelectTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu);

  void AddSeekableTextCmd(const std::string& name_prefix,
                          editor::RawSeekableTextFunc func,
                          editor::TextUnit unit,
                          editor::SeekType seek,
                          int menu,
                          bool change_text);

  void AddVoidCmd(const char* name, editor::RawVoidFunc func, int menu);

  bool LoadBinding();

  bool LoadFileTypes();

  // Load icons for top level windows.
  void LoadIcons();

private:
  FILE* log_file_;

#if JIL_SINGLE_INSTANCE
  wxSingleInstanceChecker* instance_checker_;
  wxServer* server_;
#endif  // JIL_SINGLE_INSTANCE

  // Files specified via command line argument.
  // TODO: Avoid this.
  wxArrayString cmdline_files_;

  Session session_;

  // Application options.
  Options options_;

  // Global editor options.
  editor::Options editor_options_;

  // Theme names.
  std::vector<wxString> theme_names_;

  editor::SharedTheme theme_;
  editor::Style* style_;

  FtConfig ft_config_;

  std::vector<editor::FtPlugin*> ft_plugins_;

  editor::Binding* binding_;

  // Status fields for status line.
  std::vector<StatusBar::FieldInfo> status_fields_;

  lua_State* lua_state_;

  // Icons for top level windows.
  wxIconBundle icons_;
};

DECLARE_APP(App)

}  // namespace jil

#endif  // JIL_APP_H_
