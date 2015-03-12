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
      , flags_(0) {
  }

  FindPanelEvent(const FindPanelEvent& rhs)
      : wxCommandEvent(rhs)
      , flags_(rhs.flags_)
      , find_str_(rhs.find_str_)
      , replace_str_(rhs.replace_str_) {
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

private:
  int flags_;
  std::wstring find_str_;
  std::wstring replace_str_;
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
  // Detailed event types of kFindPanelEvent.
  enum EventType {
    kFindStringEvent = 1,
    kFindEvent,
    kReplaceEvent,
    kFindAllEvent,
    kReplaceAllEvent,
  };

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

  bool Create(wxWindow* parent, wxWindowID id);

  virtual ~FindPanel();

  virtual bool Destroy() override;

  // Override to accept no focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

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

  void OnFindText(wxCommandEvent& evt);
  void OnFindTextEnter(wxCommandEvent& evt);

  void HandleFind(bool all);
  void HandleReplace(bool all);

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

  // \param event_type See enum EventType.
  void PostEvent(int event_type,
                 const wxString& find_str,
                 const wxString& replace_str);

private:
  editor::SharedTheme theme_;

  ui::SharedButtonStyle button_style_;

  Session* session_;

  // See enum Mode.
  int mode_;

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
