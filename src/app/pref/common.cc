#include "app/pref/common.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"

namespace jil {
namespace pref {

void UpdateFlag(int& flags, int flag, bool enable) {
  if (enable) {
    flags |= flag;
  } else {
    flags &= ~flag;
  }
}

int StrToInt(const wxString& str) {
  long i = 0;
  str.ToLong(&i);
  return static_cast<int>(i);
}

int ValidateInt(int i, int min, int max) {
  if (i < min) {
    return min;
  }
  if (i > max) {
    return max;
  }
  return i;
}

wxStaticText* CreateStaticText(wxWindow* parent, const wxString& label, bool bold) {
  wxStaticText* static_text = new wxStaticText(parent, wxID_ANY, label);
  if (bold) {
    static_text->SetFont(static_text->GetFont().Bold());
  }
  return static_text;
}

wxSizer* CreateSeparator(wxWindow* parent, const wxString& label, bool bold) {
  wxStaticText* static_text = CreateStaticText(parent, label, bold);
  wxStaticLine* sep = new wxStaticLine(parent);

  wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
  hsizer->Add(static_text, wxSizerFlags().Center());
  hsizer->Add(sep, wxSizerFlags(1).Center().Border(wxLEFT));
  return hsizer;
}

wxComboBox* CreateReadonlyComboBox(wxWindow* parent, wxWindowID id) {
  return new wxComboBox(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
}

wxTextCtrl* CreateTextCtrl(wxWindow* parent,
                           wxWindowID id,
                           const wxSize& size,
                           const wxValidator& validator) {
  return new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, size, 0, validator);
}

}  // namespace pref
}  // namespace jil
