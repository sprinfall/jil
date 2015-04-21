#ifndef JIL_FIND_WINDOW_H_
#define JIL_FIND_WINDOW_H_
#pragma once

// Find & Replace window.

#include "wx/minifram.h"
#include "wx/panel.h"
#include "app/defs.h"

class wxButton;
class wxComboBox;

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
  enum Mode {
    kFindMode = 0,
    kReplaceMode
  };

public:
  FindWindow();
  FindWindow(Session* session, int mode);

  bool Create(BookFrame* book_frame, wxWindowID id);

  virtual ~FindWindow();

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

  ui::BitmapToggleButton* NewToggleButton(wxWindowID id,
                                          const wxString& bitmap,
                                          const wxString& tooltip);

  wxButton* NewButton(wxWindowID id, const wxString& label);

 private:
  Session* session_;

  BookFrame* book_frame_;

  int mode_;  // enum Mode

  // See enum FindFlag.
  int flags_;

  // For tab traversal.
  wxPanel* panel_;

  ui::BitmapToggleButton* regex_toggle_button_;
  ui::BitmapToggleButton* case_toggle_button_;
  ui::BitmapToggleButton* word_toggle_button_;
  ui::BitmapToggleButton* mode_toggle_button_;

  wxComboBox* find_combo_box_;
  wxComboBox* replace_combo_box_;

  wxButton* find_button_;
  wxButton* find_all_button_;
  wxButton* replace_button_;
  wxButton* replace_all_button_;
};

}  // namespace jil

#endif  // JIL_FIND_WINDOW_H_
