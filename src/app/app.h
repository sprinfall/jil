#ifndef JIL_APP_H_
#define JIL_APP_H_
#pragma once

#include <list>
#include <map>
#include <vector>

#include "wx/app.h"
#include "wx/arrstr.h"
#include "wx/ipc.h"
#include "wx/scopedptr.h"

#include "editor/defs.h"
#include "editor/option.h"
#include "editor/status_bar.h"
#include "editor/text_func.h"
#include "editor/theme.h"

#include "app/config.h"
#include "app/option.h"
#include "app/session.h"

class wxPreferencesEditor;
class wxSingleInstanceChecker;

// App name used for paths, config, and other places the user doesn't see.
#if defined (__WXMSW__) || defined (__WXMAC__)
#define kAppName wxT("Jil")
#else
#define kAppName wxT("jil")
#endif

// App name used in window titles, etc.
#define kAppDisplayName wxT("Jil Text")

// App version.
#define kAppVersion wxT("0.1")

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

  const std::list<wxString>& theme_names() const {
    return theme_names_;
  }

  const std::list<editor::FileType*>& file_types() const {
    return file_types_;
  }

  const editor::FileType& FileTypeFromExt(const wxString& ext) const;

  // Get ft plugin object.
  // If the ft plugin doesn't exist, it will be created.
  editor::FtPlugin* GetFtPlugin(const editor::FileType& ft);

  const std::vector<editor::StatusBar::FieldInfo>& status_fields() const {
    return status_fields_;
  }

  void ShowPreferencesEditor(wxWindow* parent);
  void DismissPreferencesEditor();

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

  void AddMoveTextCmd(editor::TextUnit unit, editor::SeekType seek, int menu);

  void AddDeleteTextCmd(editor::TextUnit unit,
                        editor::SeekType seek,
                        int menu);

  void AddScrollTextCmd(editor::TextUnit unit,
                        editor::SeekType seek,
                        int menu);

  void AddSelectTextCmd(editor::TextUnit unit,
                        editor::SeekType seek,
                        int menu);

  void AddSeekableTextCmd(const std::string& name_prefix,
                          editor::RawSeekableTextFunc func,
                          editor::TextUnit unit,
                          editor::SeekType seek,
                          int menu,
                          bool change_text);

  void AddVoidCmd(const char* name, editor::RawVoidFunc func, int menu);

  bool LoadBinding();

  bool LoadFileTypes();

  // Load lex for the file type.
  void LoadLex(editor::FtPlugin* ft_plugin);

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

  // The ID of internal file types always starts with "jil-".
  // Internal file types won't be displayed to the end user.
  std::list<editor::FileType*> internal_file_types_;

  // Example: wxT("cpp") -> { "cpp", wxT("C++") }
  typedef std::map<wxString, editor::FileType*> ExtFtMap;
  ExtFtMap ext_ft_map_;

  std::vector<editor::FtPlugin*> ft_plugins_;

  editor::Binding* binding_;

  // Status fields for status line.
  std::vector<editor::StatusBar::FieldInfo> status_fields_;

  // For preferences dialog.
  // NOTE: Both OS X and GTK+ preferences windows are modeless unlike
  // Windows options dialogs that are typically modal.
  wxScopedPtr<wxPreferencesEditor> pref_editor_;
};

DECLARE_APP(App)

}  // namespace jil

#endif  // JIL_APP_H_
