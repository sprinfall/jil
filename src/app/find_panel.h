#ifndef JIL_FIND_PANEL_H_
#define JIL_FIND_PANEL_H_
#pragma once

// Find & replace panel.

#include "wx/control.h"
#include "wx/panel.h"
#include "ui/button_style.h"
#include "editor/theme.h"
#include "app/defs.h"

class wxComboBox;
class wxStaticText;
class wxTextCtrl;

namespace jil {

namespace ui {
class BitmapButton;
class BitmapToggleButton;
class Label;
class TextButton;
}  // namespace ui

class BookFrame;
class Session;

////////////////////////////////////////////////////////////////////////////////

BEGIN_DECLARE_EVENT_TYPES()
// Check GetInt(), which returns FindPanel::EventType, for the details.
DECLARE_EVENT_TYPE(kFindPanelEvent, 0)
END_DECLARE_EVENT_TYPES()

class FindPanelEvent : public wxCommandEvent {
public:
  FindPanelEvent(int id)
      : wxCommandEvent(kFindPanelEvent, id)
      , flags_(0)
      , location_(kCurrentPage) {
  }

  FindPanelEvent(const FindPanelEvent& rhs)
      : wxCommandEvent(rhs)
      , flags_(rhs.flags_)
      , find_str_(rhs.find_str_)
      , replace_str_(rhs.replace_str_)
      , location_(rhs.location_) {
  }

  virtual wxEvent* Clone() const {
    return new FindPanelEvent(*this);
  }

  int flags() const {
    return flags_;
  }
  void set_flags(int flags) {
    flags_ = flags;
  }

  const std::wstring& find_str() const {
    return find_str_;
  }
  void set_find_str(const std::wstring& find_str) {
    find_str_ = find_str;
  }

  const std::wstring& replace_str() const {
    return replace_str_;
  }
  void set_replace_str(const std::wstring& replace_str) {
    replace_str_ = replace_str;
  }

  FindLocation location() const {
    return location_;
  }
  void set_location(FindLocation location) {
    location_ = location;
  }

private:
  int flags_;
  std::wstring find_str_;
  std::wstring replace_str_;
  FindLocation location_;
};

typedef void (wxEvtHandler::*FindPanelEventFunction)(FindPanelEvent&);

#define FindPanelEventHandler(func)\
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(jil::FindPanelEventFunction, &func)

#define EVT_FIND_PANEL(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(jil::kFindPanelEvent, id, -1,\
  FindPanelEventHandler(func), (wxObject*)NULL),

////////////////////////////////////////////////////////////////////////////////

class FindPanel : public wxPanel {
  DECLARE_DYNAMIC_CLASS(FindPanel)
  DECLARE_EVENT_TABLE()

public:
  enum Mode {
    kFindMode = 0,
    kReplaceMode
  };

  // Detailed event types of kFindPanelEvent.
  enum EventType {
    // Find string is changed, e.g., when user is typing.
    kFindStrEvent = 1,

    // Enter key is pressed for Find combo box or Find button is clicked.
    // This normally means Find Next.
    kFindEvent,

    // Find All button is clicked.
    kFindAllEvent,

    // Enter key is pressed for Replace combo box or Replace button is clicked.
    kReplaceEvent,

    // Replace All button is clicked.
    kReplaceAllEvent,

    // Some part of find panel is shown or hidden.
    // Tell the book frame to re-layout.
    kLayoutEvent,
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
    IMAGE_LOCATION = 0,
    IMAGE_CASE_SENSITIVE,
    IMAGE_MATCH_WORD,
    IMAGE_USE_REGEX,
    IMAGE_ADD,
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

  void OnLocationButtonClick(wxCommandEvent& evt);
  void OnAddFolderButtonClick(wxCommandEvent& evt);

  void OnUseRegexToggle(wxCommandEvent& evt);
  void OnCaseSensitiveToggle(wxCommandEvent& evt);
  void OnMatchWordToggle(wxCommandEvent& evt);

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

  void AddOptionButtons(wxSizer* hsizer);

  void ShowReplace(bool show);
  void ShowFolders(bool show);

  void InitButtonStyle();

  ui::BitmapButton* NewBitmapButton(int id, ImageId image_id);
  ui::BitmapToggleButton* NewBitmapToggleButton(int id, ImageId image_id);
  ui::TextButton* NewTextButton(int id, const wxString& label);

  // \param event_type See enum EventType.
  void PostEvent(int event_type,
                 const wxString& find_str,
                 const wxString& replace_str);

private:
  BookFrame* book_frame_;

  editor::SharedTheme theme_;
  ui::SharedButtonStyle button_style_;

  Session* session_;

  int mode_;  // enum Mode

  // See enum FindFlag.
  int flags_;

  FindLocation location_;

  wxStaticText* folders_label_;
  wxTextCtrl* folders_text_ctrl_;
  ui::BitmapButton* add_folder_button_;

  ui::BitmapButton* location_button_;
  ui::BitmapToggleButton* use_regex_tbutton_;
  ui::BitmapToggleButton* case_sensitive_tbutton_;
  ui::BitmapToggleButton* match_word_tbutton_;

  wxComboBox* find_combobox_;
  wxComboBox* replace_combobox_;

  ui::TextButton* find_button_;
  ui::TextButton* find_all_button_;
  ui::TextButton* replace_button_;
  ui::TextButton* replace_all_button_;
};

}  // namespace jil

#endif  // JIL_FIND_PANEL_H_
