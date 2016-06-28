#ifndef JIL_PREF_COMMON_H_
#define JIL_PREF_COMMON_H_
#pragma once

// Preference utilities.

#include "wx/gdicmn.h"
#include "wx/string.h"
#include "wx/window.h"

class wxComboBox;
class wxStaticText;
class wxSizer;
class wxTextCtrl;

namespace jil {
namespace pref {

enum Id {
  ID_PREF_BEGIN = wxID_HIGHEST + 100,

  ID_FONT_LIST_CTRL,
  ID_FONT_NAME_COMBOBOX,
  ID_FONT_SIZE_COMBOBOX,
  ID_FONT_USE_DEFAULT_BUTTON,

  ID_TAB_STOP_TEXTCTRL,
  ID_EXPAND_TAB_CHECKBOX,
  ID_GUESS_CHECKBOX,
};

static const wxSize kMinComboBoxSize(120, -1);
static const wxSize kNumTextSize(100, -1);
static const wxSize kSmallNumTextSize(60, -1);
static const wxSize kStrTextSize(280, -1);

// Add or remove a flag from the flags.
void UpdateFlag(int& flags, int flag, bool enable);

static inline wxString IntToStr(int i) {
  return wxString::Format(wxT("%d"), i);
}

int StrToInt(const wxString& str);

int ValidateInt(int i, int min, int max);

wxStaticText* CreateStaticText(wxWindow* parent, const wxString& label, bool bold = false);

wxSizer* CreateSeparator(wxWindow* parent, const wxString& label, bool bold = false);

wxComboBox* CreateReadonlyComboBox(wxWindow* parent, wxWindowID id);

wxTextCtrl* CreateTextCtrl(wxWindow* parent,
                           wxWindowID id,
                           const wxSize& size = wxDefaultSize,
                           const wxValidator& validator = wxDefaultValidator);

}  // namespace pref
}  // namespace jil

#endif  // JIL_PREF_COMMON_H_
