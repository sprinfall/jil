#ifndef JIL_FIND_PANEL_H_
#define JIL_FIND_PANEL_H_
#pragma once

// Find & replace panel.
// TODO: Allow to set font.

#include <list>
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
class FindTextCtrl;
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

  bool Create(BookFrame* book_frame, wxWindowID id);

  virtual ~FindPanel();

  virtual bool Destroy() override;

  // Override to accept no focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

  virtual void SetFocus() override;

  // Override to also set font to child windows (text controls).
  virtual bool SetFont(const wxFont& font) override;

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

  void ReapplyTheme();

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

  // Handle find text change, including:
  // - Enable find & replace buttons;
  // - Validate find text and set its foreground color;
  // - Start an incremental find if the location is current page.
  // Not only called when find text is really changed, but also when
  // Use Regex option is switched.
  void HandleFindTextChange();

  // Find the given string incrementally in the current page.
  // Please check the location before the call.
  void FindIncrementally(const wxString& find_wxstr);

  void OnFindTextHistory(wxCommandEvent& evt);
  void OnReplaceTextHistory(wxCommandEvent& evt);
  void ShowHistoryMenu(const std::list<wxString>& history_strings,
                       int id_begin,
                       wxTextCtrl* text_ctrl);

  void OnFindTextHistoryMenu(wxCommandEvent& evt);
  void OnReplaceTextHistoryMenu(wxCommandEvent& evt);
  void HandleHistoryMenu(const std::list<wxString>& history_strings,
                         int menu_id,
                         wxTextCtrl* text_ctrl);

  void HandleFind();
  void HandleReplace(bool all);

  void SetLocation(FindLocation location);

  // Check if the given regex is valid or not.
  bool IsRegexValid(const std::wstring& re_str) const;

  bool IsFindStringValid(const wxString& find_wxstr, bool empty_as_valid) const;

  // Set the foreground color of the find text ctrl.
  void SetFindTextCtrlFgColor(bool valid);

  // Get the folders from control.
  wxArrayString GetFolders() const;

private:
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

  // Enable or disable find and replace buttons.
  void EnableButtons(bool enable);

  void InitButtonStyle();

  ui::BitmapButton* NewBitmapButton(int id);
  ui::BitmapToggleButton* NewBitmapToggleButton(int id);
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

  FindTextCtrl* find_text_ctrl_;
  FindTextCtrl* replace_text_ctrl_;

  ui::TextButton* find_button_;
  ui::TextButton* find_all_button_;
  ui::TextButton* replace_button_;
  ui::TextButton* replace_all_button_;

  wxSize bitmap_button_best_size_;

  // Foreground colors for find text control.
  // When find using regex, the text will highlighted if the regex is invalid.
  wxColour fg_color_;
  wxColour invalid_fg_color_;
};

}  // namespace jil

#endif  // JIL_FIND_PANEL_H_
