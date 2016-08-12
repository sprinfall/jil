#ifndef JIL_FIND_PANEL_H_
#define JIL_FIND_PANEL_H_
#pragma once

// Find & replace panel.
// TODO: Allow to set font.

#include "wx/control.h"
#include "wx/panel.h"
#include "ui/button_style.h"
#include "editor/theme.h"
#include "app/compile_config.h"
#include "app/defs.h"

class wxComboBox;
class wxStaticBitmap;
class wxTextCtrl;

namespace jil {

namespace ui {
class BitmapButton;
class BitmapToggleButton;
class Label;
class Separator;
class TextButton;
}  // namespace ui

class BookFrame;
class Session;

////////////////////////////////////////////////////////////////////////////////

BEGIN_DECLARE_EVENT_TYPES()
// Event posted when find panel changes its layout.
DECLARE_EVENT_TYPE(kFindPanelLayoutEvent, 0)
END_DECLARE_EVENT_TYPES()

#define EVT_FIND_PANEL_LAYOUT(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(jil::kFindPanelLayoutEvent, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

////////////////////////////////////////////////////////////////////////////////

class FindPanel : public wxPanel {
  DECLARE_DYNAMIC_CLASS(FindPanel)
  DECLARE_EVENT_TABLE()

public:
  enum Mode {
    kFindMode = 0,
    kReplaceMode
  };

  enum ColorId {
    COLOR_FG = 0,
    COLOR_BG_TOP,
    COLOR_BG_BOTTOM,
    COLOR_BORDER_OUTER,
    COLOR_BORDER_INNER,
    COLORS,
  };

  enum ImageId {
    IMAGE_HISTORY = 0,
#if JIL_BMP_BUTTON_FIND_OPTIONS
    IMAGE_LOCATION,
    IMAGE_CASE_SENSITIVE,
    IMAGE_MATCH_WORD,
    IMAGE_USE_REGEX,
#endif  // JIL_BMP_BUTTON_FIND_OPTIONS
    IMAGE_FOLDERS,
    IMAGE_ADD_FOLDER,
    IMAGES,
  };

  enum ThemeId {
    THEME_BUTTON = 0,
    THEMES
  };

public:
  FindPanel();
  FindPanel(Session* session, int mode);

  bool Create(BookFrame* book_frame, wxWindowID id);

  virtual ~FindPanel();

  virtual bool Destroy() override;

  // Override to accept no focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

  virtual void SetFocus() override;

  void set_theme(const editor::SharedTheme& theme) {
    theme_ = theme;
  }

  void set_session(Session* session) {
    session_ = session;
  }

  int mode() const {
    return mode_;
  }
  void set_mode(int mode) {
    mode_ = mode;
  }

  int flags() const {
    return flags_;
  }

  FindLocation location() const {
    return location_;
  }

  void UpdateLayout();

  void SetFindString(const wxString& find_string);

protected:
  void OnPaint(wxPaintEvent& evt);

  void OnMenuCurrentPage(wxCommandEvent& evt);
  void OnMenuAllPages(wxCommandEvent& evt);
  void OnMenuFolders(wxCommandEvent& evt);

  void OnAddFolderButtonClick(wxCommandEvent& evt);

  void OnLocationClick(wxCommandEvent& evt);

#if JIL_BMP_BUTTON_FIND_OPTIONS
  void OnUseRegexToggle(wxCommandEvent& evt);
  void OnCaseSensitiveToggle(wxCommandEvent& evt);
  void OnMatchWordToggle(wxCommandEvent& evt);
#else
  void OnOptionsLabel(wxCommandEvent& evt);
  void OnMenuUseRegex(wxCommandEvent& evt);
  void OnMenuCaseSensitive(wxCommandEvent& evt);
  void OnMenuMatchWord(wxCommandEvent& evt);
#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

  void OnFind(wxCommandEvent& evt);
  void OnFindAll(wxCommandEvent& evt);
  void OnReplace(wxCommandEvent& evt);
  void OnReplaceAll(wxCommandEvent& evt);

  void OnFindText(wxCommandEvent& evt);
  void OnFindTextEnter(wxCommandEvent& evt);

  void HandleFind();
  void HandleReplace(bool all);

  void SetLocation(FindLocation location);

private:
  // Initialize find combobox with find history.
  void InitFindComboBox();

  // Initialize replace combobox with replace history.
  void InitReplaceComboBox();

  // Append a folder to folders text ctrl.
  void AddFolder(const wxString& folder);

  // Add a string to find history and find combobox.
  void AddFindString(const wxString& string);

  // Add a string to replace history and replace combobox.
  void AddReplaceString(const wxString& string);

  void LayoutAsFind();
  void LayoutAsReplace();

  void CommonLayout(bool with_replace);
  wxSizer* CommonLayoutHead(bool with_replace);
  wxSizer* CommonLayoutBody(bool with_replace);
  wxSizer* CommonLayoutFoot(bool with_replace);

  void ShowReplace(bool show);
  void ShowFolders(bool show);

  void InitButtonStyle();

  ui::BitmapButton* NewBitmapButton(int id, ImageId image_id);
  ui::BitmapToggleButton* NewBitmapToggleButton(int id, ImageId image_id);
  ui::TextButton* NewTextButton(int id, const wxString& label);

  void PostLayoutEvent();

private:
  BookFrame* book_frame_;

  editor::SharedTheme theme_;
  ui::SharedButtonStyle button_style_;

  Session* session_;

  int mode_;  // enum Mode

  // See enum FindFlag.
  int flags_;

  FindLocation location_;

  wxStaticBitmap* folders_bitmap_;
  wxTextCtrl* folders_text_ctrl_;
  ui::BitmapButton* add_folder_button_;

#if JIL_BMP_BUTTON_FIND_OPTIONS
  ui::BitmapButton* location_button_;
  ui::BitmapToggleButton* use_regex_tbutton_;
  ui::BitmapToggleButton* case_sensitive_tbutton_;
  ui::BitmapToggleButton* match_word_tbutton_;
#else
  ui::Label* location_label_;
  ui::Separator* sep_;
  ui::Label* options_label_;
#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

  wxTextCtrl* find_text_ctrl_;
  ui::BitmapButton* find_history_button_;

  wxTextCtrl* replace_text_ctrl_;
  ui::BitmapButton* replace_history_button_;

  ui::TextButton* find_button_;
  ui::TextButton* find_all_button_;
  ui::TextButton* replace_button_;
  ui::TextButton* replace_all_button_;
};

}  // namespace jil

#endif  // JIL_FIND_PANEL_H_
