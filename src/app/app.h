#ifndef JIL_APP_H_
#define JIL_APP_H_
#pragma once

#include <vector>
#include <list>
#include <map>
#include "wx/app.h"
#include "wx/ipc.h"
#include "wx/arrstr.h"
#include "editor/defs.h"
#include "editor/option.h"
#include "editor/status_bar.h"
#include "editor/text_func.h"
#include "editor/theme.h"
#include "app/config.h"
#include "app/session.h"
#include "app/option.h"

class wxSingleInstanceChecker;

// App name used internally.
#define kAppName wxT("jil")

// App name used in window titles, etc.
#define kAppDisplayName wxT("Jil Text")

// App version.
#define kAppVersion wxT("0.1")

namespace jil {

namespace editor {
class Style;
class FtPlugin;
class Binding;
}

class BookFrame;
class Session;

typedef void (*FtPluginLoader)(editor::FtPlugin* ft_plugin);

class App : public wxApp {
 public:
  App();
  virtual ~App();

  virtual bool OnInit() override;
  virtual int OnExit() override;

  const Options& options() const {
    return options_;
  }

  const std::list<wxString>& theme_names() const {
    return theme_names_;
  }

  const editor::FileType& FileTypeFromExt(const wxString& ext) const;

  // Get ft plugin object.
  // If the ft plugin doesn't exist, it will be created.
  editor::FtPlugin* GetFtPlugin(const editor::FileType& ft,
                                FtPluginLoader ft_plugin_loader = NULL);

  const std::vector<editor::StatusBar::FieldInfo>& status_fields() const {
    return status_fields_;
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

  wxString ResourceDir(const wxString& dir) const;
  wxString ResourceDir(const wxString& dir, const wxString& dir2) const;

  wxString ResourceFile(const wxString& file) const;
  wxString ResourceFile(const wxString& dir, const wxString& file) const;

  void LoadStatusFields();

  void LoadOptions();

  bool LoadTheme();

  // Init text and void commands.
  void InitCommands();

  void AddTextCmd(const char* name,
                  editor::RawTextFunc func,
                  bool change_text,
                  int menu);

  void AddSeekableTextCmd(const char* name,
                          editor::RawSeekableTextFunc func,
                          editor::TextUnit unit,
                          editor::SeekType seek,
                          int menu);

  void AddVoidCmd(const char* name, editor::RawVoidFunc func, int menu);

  bool LoadBinding();

  void UseDefaultStatusFields();

  bool LoadFileTypes();

  // Load lex for the file type.
  void LoadLex(editor::FtPlugin* ft_plugin);

  // Open the last opened files, activate the last active file.
  void RestoreLastOpenedFiles(BookFrame* book_frame);

 private:
  wxSingleInstanceChecker* instance_checker_;
  wxServer* server_;

  FILE* log_file_;

  // Files specified via command line argument.
  // TODO: Avoid this.
  wxArrayString cmdline_files_;

  Session session_;

  // Application options.
  Options options_;

  // Global editor options.
  editor::Options editor_options_;

  // A list of theme names.
  std::list<wxString> theme_names_;

  editor::SharedTheme theme_;
  editor::Style* style_;

  std::list<editor::FileType*> file_types_;

  // Example: wxT("cpp") -> { "cpp", wxT("C++") }
  typedef std::map<wxString, editor::FileType*> ExtFtMap;
  ExtFtMap ext_ft_map_;

  std::vector<editor::FtPlugin*> ft_plugins_;

  editor::Binding* binding_;

  // Status fields for status line.
  std::vector<editor::StatusBar::FieldInfo> status_fields_;
};

DECLARE_APP(App)

}  // namespace jil

#endif  // JIL_APP_H_
