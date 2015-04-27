#include "app/find_window.h"

#include "wx/button.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/checkbox.h"
#include "wx/sizer.h"
#include "wx/statline.h"
#include "wx/stattext.h"
#include "wx/frame.h"  // wxFRAME_TOOL_WINDOW
#include "wx/log.h"

#include "ui/bitmap_toggle_button.h"

#include "editor/text_buffer.h"

#include "app/id.h"
#include "app/session.h"
#include "app/skin.h"
#include "app/text_page.h"
#include "app/util.h"

#define kTitle _("Find & Replace")

#define kTrUseRegex _("Use regular &expression")
#define kTrCaseSensitive _("&Case sensitive")
#define kTrMatchWord _("Match whole &word")
//#define kTrSearchReversely _("Search reversely")
#define kTrSwitchMode _("Switch between find and replace modes")

namespace jil {

DEFINE_EVENT_TYPE(kFindWindowEvent)

IMPLEMENT_DYNAMIC_CLASS(FindWindow, wxMiniFrame);

BEGIN_EVENT_TABLE(FindWindow, wxMiniFrame)
EVT_ACTIVATE    (FindWindow::OnActivate)
EVT_CLOSE       (FindWindow::OnClose)
EVT_CHAR_HOOK   (FindWindow::OnKeyDownHook)

EVT_TOGGLEBUTTON(ID_FIND_MODE_TOGGLE_BUTTON, FindWindow::OnModeToggleButton)

EVT_TEXT(ID_FIND_COMBOBOX, FindWindow::OnFindComboBoxText)

EVT_COMBOBOX(ID_LOCATION_COMBOBOX,  FindWindow::OnLocationComboBox)

EVT_TOGGLEBUTTON(ID_OPTIONS_TOGGLE_BUTTON, FindWindow::OnOptionsToggleButton)

EVT_CHECKBOX(ID_USE_REGEX_CHECKBOX,         FindWindow::OnUseRegexCheckBox)
EVT_CHECKBOX(ID_CASE_SENSITIVE_CHECKBOX,    FindWindow::OnCaseSensitiveCheckBox)
EVT_CHECKBOX(ID_MATCH_WORD_CHECKBOX,        FindWindow::OnMatchWordCheckBox)

EVT_BUTTON(ID_FIND_BUTTON,          FindWindow::OnFind)
EVT_BUTTON(ID_FIND_ALL_BUTTON,      FindWindow::OnFindAll)
EVT_BUTTON(ID_REPLACE_BUTTON,       FindWindow::OnReplace)
EVT_BUTTON(ID_REPLACE_ALL_BUTTON,   FindWindow::OnReplaceAll)
END_EVENT_TABLE()

FindWindow::FindWindow()
    : session_(NULL), mode_(kFindMode), flags_(0), location_(kCurrentPage) {
}

FindWindow::FindWindow(Session* session, int mode)
    : session_(session), mode_(mode), flags_(0), location_(kCurrentPage) {
}

bool FindWindow::Create(wxWindow* parent, wxWindowID id) {
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();
  location_ = session_->find_location();

  long style = wxCLOSE_BOX|wxCAPTION|wxCLIP_CHILDREN|wxRESIZE_BORDER|wxFRAME_TOOL_WINDOW|wxWANTS_CHARS;
  if (!wxMiniFrame::Create(parent, id, kTitle, wxDefaultPosition, wxDefaultSize, style)) {
    return false;
  }

  // Panel has style wxTAB_TRAVERSAL by default.
  panel_ = new wxPanel(this);
  panel_->SetBackgroundColour(*wxWHITE);  // TODO

  //----------------------------------------------------------------------------

  mode_toggle_button_ = NewToggleButton(ID_FIND_MODE_TOGGLE_BUTTON, wxT("fw_mode"), kTrSwitchMode);

  find_label_ = new wxStaticText(panel_, wxID_ANY, _("Find:"));

  find_combo_box_ = new wxComboBox(panel_, ID_FIND_COMBOBOX);

  // Initialize find combobox with find history.
  const std::list<wxString>& find_strings = session_->find_strings();
  {
    std::list<wxString>::const_iterator it = find_strings.begin();
    for (; it != find_strings.end(); ++it) {
      find_combo_box_->Append(*it);
    }
  }

  if (!find_combo_box_->IsListEmpty()) {
    find_combo_box_->Select(0);
  }

  //----------------------------------------------------------------------------

  replace_label_ = new wxStaticText(panel_, wxID_ANY, _("Replace with:"));

  replace_combo_box_ = new wxComboBox(panel_, ID_REPLACE_COMBOBOX);

  // Initialize replace combobox with replace history.
  const std::list<wxString>& replace_strings = session_->replace_strings();
  {
    std::list<wxString>::const_iterator it = replace_strings.begin();
    for (; it != replace_strings.end(); ++it) {
      replace_combo_box_->Append(*it);
    }
  }

  if (!replace_combo_box_->IsListEmpty()) {
    replace_combo_box_->Select(0);
  }

  //----------------------------------------------------------------------------

  location_label_ = new wxStaticText(panel_, wxID_ANY, _("In:"));

  wxString locations[] = { _("Current Page"), _("All Pages"), _("Selection") };
  location_combo_box_ = new wxComboBox(panel_,
                                       ID_LOCATION_COMBOBOX,
                                       wxEmptyString,
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       3,
                                       locations,
                                       wxCB_READONLY);
  location_combo_box_->Select(location_);

  //----------------------------------------------------------------------------

  options_toggle_button_ = NewToggleButton(ID_OPTIONS_TOGGLE_BUTTON, wxT("fw_options"), wxEmptyString);
  options_toggle_button_->set_toggle(session_->show_options());

  options_label_ = new wxStaticText(panel_, wxID_ANY, _("Options"));
  options_line_ = new wxStaticLine(panel_);

  use_regex_check_box_ = new wxCheckBox(panel_, ID_USE_REGEX_CHECKBOX, kTrUseRegex);
  case_sensitive_check_box_ = new wxCheckBox(panel_, ID_CASE_SENSITIVE_CHECKBOX, kTrCaseSensitive);
  match_word_check_box_ = new wxCheckBox(panel_, ID_MATCH_WORD_CHECKBOX, kTrMatchWord);

  use_regex_check_box_->SetValue(GetBit(flags_, kFind_UseRegex));
  case_sensitive_check_box_->SetValue(GetBit(flags_, kFind_CaseSensitive));
  match_word_check_box_->SetValue(GetBit(flags_, kFind_MatchWord));

  //----------------------------------------------------------------------------

  find_button_ = NewButton(ID_FIND_BUTTON, _("&Find"));
  find_all_button_ = NewButton(ID_FIND_ALL_BUTTON, _("Find All"));

  replace_button_ = NewButton(ID_REPLACE_BUTTON, _("&Replace"));
  replace_all_button_ = NewButton(ID_REPLACE_ALL_BUTTON, _("Replace All"));

  find_button_->SetDefault();  // Set default for ENTER key.

  UpdateButtonState();

  //----------------------------------------------------------------------------
  // Layout

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  panel_->SetSizer(vsizer);

  ShowOptions(session_->show_options());

  UpdateLayout();

  return true;
}

FindWindow::~FindWindow() {
}

void FindWindow::UpdateLayout() {
  bool show_replace = mode_ == kReplaceMode;

  replace_label_->Show(show_replace);
  replace_combo_box_->Show(show_replace);
  replace_button_->Show(show_replace);
  replace_all_button_->Show(show_replace);

  wxSizer* top_vsizer = panel_->GetSizer();
  top_vsizer->Clear(false);

  {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(find_label_, wxSizerFlags().Bottom().Border(wxBOTTOM, 2));
    hsizer->AddStretchSpacer(1);
    hsizer->Add(mode_toggle_button_, wxSizerFlags().Border(wxTOP|wxBOTTOM));
    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxLR));
  }

  top_vsizer->Add(find_combo_box_, wxSizerFlags().Expand().Border(wxLR));

  if (show_replace) {
    top_vsizer->Add(replace_label_, wxSizerFlags().Border(wxLTR));
    top_vsizer->AddSpacer(2);
    top_vsizer->Add(replace_combo_box_, wxSizerFlags().Expand().Border(wxLR));
  }

  top_vsizer->Add(location_label_, wxSizerFlags().Border(wxLTR));
  top_vsizer->AddSpacer(2);
  top_vsizer->Add(location_combo_box_, wxSizerFlags().Expand().Border(wxLRB));

  {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(options_label_, wxSizerFlags().Center());
    hsizer->Add(options_line_, wxSizerFlags(1).Center().Border(wxLEFT));
    hsizer->Add(options_toggle_button_, wxSizerFlags().Center().Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  {
    wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    vsizer->Add(use_regex_check_box_, wxSizerFlags());
    vsizer->Add(case_sensitive_check_box_, wxSizerFlags().Border(wxTOP));
    vsizer->Add(match_word_check_box_, wxSizerFlags().Border(wxTOP));
    top_vsizer->Add(vsizer, wxSizerFlags().Expand().Border(wxALL));
  }

  {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(find_button_);
    hsizer->Add(find_all_button_, wxSizerFlags().Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Right().Border(show_replace ? wxLTR : wxALL));
  }

  if (show_replace) {
    wxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    hsizer->Add(replace_button_);
    hsizer->Add(replace_all_button_, wxSizerFlags().Border(wxLEFT));
    top_vsizer->Add(hsizer, wxSizerFlags().Right().Border(wxALL));
  }

  top_vsizer->Layout();
  UpdateSizes();
}

void FindWindow::SetFindString(const wxString& find_string) {
  find_combo_box_->SetValue(find_string);
  AddFindString(find_string);
}

// Update control values on activate.
void FindWindow::OnActivate(wxActivateEvent& evt) {
  // Focus and select the text so the user can change it directly.
  find_combo_box_->SetFocus();
  find_combo_box_->SelectAll();
}

void FindWindow::OnClose(wxCloseEvent& evt) {
  session_->set_find_window_rect(GetScreenRect());
  session_->set_find_flags(flags_);
  session_->set_find_location(location_);
  session_->set_show_options(options_toggle_button_->toggle());

  evt.Skip();
}

void FindWindow::OnKeyDownHook(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_ESCAPE) {
    Close();
  } else {
    evt.Skip();
  }
}

void FindWindow::OnModeToggleButton(wxCommandEvent& evt) {
  if (evt.IsChecked()) {
    mode_ = kReplaceMode;
  } else {
    mode_ = kFindMode;
  }

  UpdateLayout();
}

void FindWindow::OnFindComboBoxText(wxCommandEvent& evt) {
  UpdateButtonState();
}

void FindWindow::OnLocationComboBox(wxCommandEvent& evt) {
  location_ = static_cast<FindLocation>(evt.GetSelection());
}

void FindWindow::OnOptionsToggleButton(wxCommandEvent& evt) {
  ShowOptions(evt.IsChecked());
}

void FindWindow::ShowOptions(bool show) {
  use_regex_check_box_->Show(show);
  case_sensitive_check_box_->Show(show);
  match_word_check_box_->Show(show);

  Layout();
  UpdateSizes();
}

void FindWindow::OnUseRegexCheckBox(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_UseRegex, evt.IsChecked());
}

void FindWindow::OnCaseSensitiveCheckBox(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_CaseSensitive, evt.IsChecked());
}

void FindWindow::OnMatchWordCheckBox(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_MatchWord, evt.IsChecked());
}

void FindWindow::OnFind(wxCommandEvent& evt) {
  wxString find_str = find_combo_box_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);
    PostEvent(kFindEvent, find_str, wxEmptyString);
  }
}

void FindWindow::OnFindAll(wxCommandEvent& evt) {
  wxString find_str = find_combo_box_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);
    PostEvent(kFindAllEvent, find_str, wxEmptyString);
  }
}

void FindWindow::OnReplace(wxCommandEvent& evt) {
  wxString find_str = find_combo_box_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    wxString replace_str = replace_combo_box_->GetValue();
    if (!replace_str.IsEmpty()) {
      AddReplaceString(replace_str);
    }

    PostEvent(kReplaceEvent, find_str, replace_str);
  }
}

void FindWindow::OnReplaceAll(wxCommandEvent& evt) {
  wxString find_str = find_combo_box_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    wxString replace_str = replace_combo_box_->GetValue();
    if (!replace_str.IsEmpty()) {
      AddReplaceString(replace_str);
    }

    PostEvent(kReplaceAllEvent, find_str, replace_str);
  }
}

void FindWindow::AddFindString(const wxString& string) {
  if (session_->AddFindString(string)) {
    // The find string is new. Simply push to find combobox at front.
    find_combo_box_->Insert(string, 0);
  } else {
    // Move the find string to front in the find combobox.
    int old_index = find_combo_box_->FindString(string, true);
    if (old_index != wxNOT_FOUND) {
      find_combo_box_->Delete(old_index);
    }
    find_combo_box_->Insert(string, 0);
    find_combo_box_->Select(0);
  }
}

void FindWindow::AddReplaceString(const wxString& string) {
  if (session_->AddReplaceString(string)) {
    // The replace string is new. Simply push to replace combobox at front.
    replace_combo_box_->Insert(string, 0);
  } else {
    // Move the find string to front in the replace combobox.
    int old_index = replace_combo_box_->FindString(string, true);
    if (old_index != wxNOT_FOUND) {
      replace_combo_box_->Delete(old_index);
    }
    replace_combo_box_->Insert(string, 0);
    replace_combo_box_->Select(0);
  }
}

void FindWindow::UpdateSizes() {
  wxSizer* vsizer = panel_->GetSizer();

  wxSize min_client_size = vsizer->GetMinSize();
  SetMinClientSize(min_client_size);

  wxSize client_size = min_client_size;
  client_size.x = GetClientSize().x;
  SetClientSize(client_size);

  vsizer->Layout();
}

void FindWindow::UpdateButtonState() {
  bool enabled = !find_combo_box_->GetValue().IsEmpty();

  if (find_button_->IsEnabled() != enabled) {
    find_button_->Enable(enabled);
    find_all_button_->Enable(enabled);
    replace_button_->Enable(enabled);
    replace_all_button_->Enable(enabled);
  }
}

ui::BitmapToggleButton* FindWindow::NewToggleButton(wxWindowID id, const wxString& bitmap, const wxString& tooltip) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(panel_, id);
  button->SetBitmaps(skin::GetIcon(bitmap), skin::GetIcon(bitmap + wxT("_toggle")));
  button->SetToolTip(tooltip);
  return button;
}

wxButton* FindWindow::NewButton(int id, const wxString& label) {
  wxButton* button = new wxButton(panel_, id, label);
  return button;
}

void FindWindow::PostEvent(int event_type, const wxString& find_str, const wxString& replace_str) {
  FindWindowEvent evt(GetId());
  evt.SetEventObject(this);
  evt.SetInt(event_type);
  evt.set_flags(flags_);
  evt.set_location(location_);
  evt.set_find_str(find_str.ToStdWstring());
  evt.set_replace_str(replace_str.ToStdWstring());
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace jil
