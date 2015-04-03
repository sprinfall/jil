#ifndef JIL_FIND_WINDOW_H_
#define JIL_FIND_WINDOW_H_
#pragma once

// Find & Replace window.

#include "wx/minifram.h"
#include "wx/panel.h"
#include "ui/button_style.h"
#include "editor/theme.h"
#include "app/defs.h"

class wxComboBox;
class wxButton;

namespace jil {

namespace ui {
class BitmapToggleButton;
}  // namespace ui
class Session;

class BookFrame;

class FindWindow : public wxMiniFrame {
  DECLARE_DYNAMIC_CLASS(FindWindow)
  DECLARE_EVENT_TABLE()

public:
  enum ThemeId {
    BUTTON = 0,
    THEMES
  };

  enum Mode {
    kFindMode = 0,
    kReplaceMode
  };

public:
  FindWindow();
  FindWindow(Session* session, int mode);

  bool Create(BookFrame* book_frame, wxWindowID id);

  virtual ~FindWindow();

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
  void OnActivate(wxActivateEvent& evt);
  void OnClose(wxCloseEvent& evt);
  void OnKeyDownHook(wxKeyEvent& evt);

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

  void UpdateSizes();

  void LayoutAsFind();
  void LayoutAsReplace();

  void InitButtonStyle();

  ui::BitmapToggleButton* NewToggleButton(int id, const wxString& bitmap);

  wxButton* NewTextButton(wxWindowID id, const wxString& label);

 private:
  Session* session_;

  BookFrame* book_frame_;

  editor::SharedTheme theme_;

  ui::SharedButtonStyle button_style_;

  int mode_;  // enum Mode

  // See enum FindFlag.
  int flags_;

  // For tab traversal.
  wxPanel* panel_;

  ui::BitmapToggleButton* use_regex_toggle_;
  ui::BitmapToggleButton* case_sensitive_toggle_;
  ui::BitmapToggleButton* match_whole_word_toggle_;
  //ui::BitmapToggleButton* search_reversely_toggle_;
  ui::BitmapToggleButton* mode_toggle_;

  wxComboBox* find_combobox_;
  wxComboBox* replace_combobox_;

  wxButton* find_button_;
  wxButton* find_all_button_;
  wxButton* replace_button_;
  wxButton* replace_all_button_;
};

}  // namespace jil

#endif  // JIL_FIND_WINDOW_H_
