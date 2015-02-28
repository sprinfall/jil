#ifndef JIL_FIND_PANEL_H_
#define JIL_FIND_PANEL_H_
#pragma once

// Find & Replace panel.

#define JIL_USE_NATIVE_BUTTON 0

#include "wx/panel.h"
#include "editor/theme.h"
#include "app/defs.h"
#include "app/text_button.h"

class wxComboBox;
class wxButton;
class wxBitmapButton;

namespace jil {

class BitmapToggleButton;
class BookFrame;
class Session;

class FindPanel : public wxPanel {
  DECLARE_DYNAMIC_CLASS(FindPanel)
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BG = 0,
    BORDER,
    COLOR_COUNT
  };

  enum ThemeId {
    BUTTON = 0,
    THEME_COUNT
  };

  enum Mode {
    kFindMode = 0,
    kReplaceMode
  };

public:
  FindPanel();
  FindPanel(Session* session, int mode);

  bool Create(BookFrame* book_frame, wxWindowID id);

  virtual ~FindPanel();

  virtual bool Show(bool show = true) override;

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

  void UpdateLayout();

  void SetFindString(const wxString& find_string);

protected:
  void OnPaint(wxPaintEvent& evt);
  //void OnClose(wxCloseEvent& evt);
  //void OnKeyDownHook(wxKeyEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);

  void OnUseRegexToggle(wxCommandEvent& evt);
  void OnCaseSensitiveToggle(wxCommandEvent& evt);
  void OnMatchWholeWordToggle(wxCommandEvent& evt);
  void OnSearchReverselyToggle(wxCommandEvent& evt);

  void OnModeToggle(wxCommandEvent& evt);

  void OnFind(wxCommandEvent& evt);
  void OnFindAll(wxCommandEvent& evt);
  void OnReplace(wxCommandEvent& evt);
  void OnReplaceAll(wxCommandEvent& evt);

private:
  // Add a string to find history and find combobox.
  void AddFindString(const wxString& string);
  // Add a string to replace history and replace combobox.
  void AddReplaceString(const wxString& string);

  void LayoutAsFind();
  void LayoutAsReplace();

  void InitButtonStyle();

  TextButton* CreateTextButton(int id, const wxString& label);

private:
  editor::SharedTheme theme_;

  Session* session_;

  BookFrame* book_frame_;

  int mode_;  // enum Mode

  // See enum FindFlag.
  int flags_;

  BitmapToggleButton* use_regex_toggle_;
  BitmapToggleButton* case_sensitive_toggle_;
  BitmapToggleButton* match_whole_word_toggle_;
  BitmapToggleButton* search_reversely_toggle_;
  BitmapToggleButton* mode_toggle_;

  wxComboBox* find_combobox_;
  wxComboBox* replace_combobox_;

#if JIL_USE_NATIVE_BUTTON
  wxButton* find_button_;
  wxButton* find_all_button_;
  wxButton* replace_button_;
  wxButton* replace_all_button_;
#else
  TextButton::SharedStyle button_style_;

  TextButton* find_button_;
  TextButton* find_all_button_;
  TextButton* replace_button_;
  TextButton* replace_all_button_;
#endif
};

}  // namespace jil

#endif  // JIL_FIND_PANEL_H_
