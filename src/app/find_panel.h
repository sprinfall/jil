#ifndef JIL_FIND_PANEL_H_
#define JIL_FIND_PANEL_H_
#pragma once

// Find & Replace panel.

#include "wx/panel.h"
#include "ui/button_style.h"
#include "editor/theme.h"
#include "app/defs.h"

class wxComboBox;

namespace jil {

namespace ui {
class BitmapToggleButton;
class TextButton;
}  // namespace ui

class BookFrame;
class Session;

class FindPanel : public wxPanel {
  DECLARE_DYNAMIC_CLASS(FindPanel)
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BG_TOP = 0,
    BG_BOTTOM,
    BORDER_OUTER,
    BORDER_INNER,
    COLORS
  };

  enum ThemeId {
    BUTTON = 0,
    THEMES
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

  virtual bool Destroy() override;

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

  virtual void SetFocus() override;

  void UpdateLayout();

  void SetFindString(const wxString& find_string);

protected:
  void OnPaint(wxPaintEvent& evt);

  void OnRegexToggle(wxCommandEvent& evt);
  void OnCaseToggle(wxCommandEvent& evt);
  void OnWholeWordToggle(wxCommandEvent& evt);

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

  ui::BitmapToggleButton* NewToggleButton(int id, const wxString& bitmap);
  ui::TextButton* NewTextButton(int id, const wxString& label);

private:
  editor::SharedTheme theme_;

  ui::SharedButtonStyle button_style_;

  Session* session_;

  BookFrame* book_frame_;

  int mode_;  // enum Mode

  // See enum FindFlag.
  int flags_;

  // Flag toggle buttons.
  ui::BitmapToggleButton* regex_toggle_button_;
  ui::BitmapToggleButton* case_toggle_button_;
  ui::BitmapToggleButton* whole_word_toggle_button_;

  wxComboBox* find_combobox_;
  wxComboBox* replace_combobox_;

  ui::TextButton* find_button_;
  ui::TextButton* find_all_button_;
  ui::TextButton* replace_button_;
  ui::TextButton* replace_all_button_;
};

}  // namespace jil

#endif  // JIL_FIND_PANEL_H_
