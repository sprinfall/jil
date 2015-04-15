#include "app/find_window.h"

#include "wx/button.h"
#include "wx/bmpbuttn.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/checkbox.h"
#include "wx/sizer.h"
#include "wx/frame.h"  // wxFRAME_TOOL_WINDOW
#include "wx/log.h"

#include "ui/bitmap_toggle_button.h"

#include "editor/text_buffer.h"

#include "app/session.h"
#include "app/text_page.h"
#include "app/book_frame.h"
#include "app/skin.h"
#include "app/util.h"

#define kTrUseRegex _("Use regular expression")
#define kTrCaseSensitive _("Case sensitive")
#define kTrMatchWholeWord _("Match whole word")
#define kTrSearchReversely _("Search reversely")
#define kTrSwitchMode _("Switch between find and replace modes")

#define kTrFind _("Find")
#define kTrReplace _("Replace")
#define kTrFindAll _("Find All")
#define kTrReplaceAll _("Replace All")

namespace jil {

const long kFindWindowStyle = wxCLOSE_BOX |\
                              wxCAPTION |\
                              wxCLIP_CHILDREN |\
                              wxRESIZE_BORDER |\
                              wxFRAME_TOOL_WINDOW |\
                              wxWANTS_CHARS;  // EVT_CHAR_HOOK needs this.

const int kPadding = 5;

const int kReplaceSizerIndex = 2;

IMPLEMENT_DYNAMIC_CLASS(FindWindow, wxMiniFrame);

BEGIN_EVENT_TABLE(FindWindow, wxMiniFrame)
EVT_ACTIVATE(FindWindow::OnActivate)
EVT_CLOSE(FindWindow::OnClose)
EVT_CHAR_HOOK(FindWindow::OnKeyDownHook)
EVT_TOGGLEBUTTON(ID_REGEX_TOGGLE_BUTTON, FindWindow::OnUseRegexToggle)
EVT_TOGGLEBUTTON(ID_CASE_TOGGLE_BUTTON, FindWindow::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON(ID_WORD_TOGGLE_BUTTON, FindWindow::OnMatchWholeWordToggle)
//EVT_TOGGLEBUTTON(kSearchReverselyToggleId, FindWindow::OnSearchReverselyToggle)
EVT_TOGGLEBUTTON(ID_FIND_MODE_TOGGLE_BUTTON, FindWindow::OnModeToggle)
EVT_BUTTON(ID_FIND_BUTTON, FindWindow::OnFind)
EVT_BUTTON(ID_FIND_ALL_BUTTON, FindWindow::OnFindAll)
EVT_BUTTON(ID_REPLACE_BUTTON, FindWindow::OnReplace)
EVT_BUTTON(ID_REPLACE_ALL_BUTTON, FindWindow::OnReplaceAll)
END_EVENT_TABLE();

FindWindow::FindWindow()
    : session_(NULL), mode_(kFindMode), book_frame_(NULL) {
}

FindWindow::FindWindow(Session* session, int mode)
    : session_(session), mode_(mode), book_frame_(NULL) {
}

bool FindWindow::Create(BookFrame* book_frame, wxWindowID id) {
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();

  book_frame_ = book_frame;

  // Note: Window title will be set later according to the mode.
  if (!wxMiniFrame::Create(book_frame,
                           id,
                           wxEmptyString,
                           wxDefaultPosition,
                           wxDefaultSize,
                           kFindWindowStyle)) {
    return false;
  }

  // Panel has style wxTAB_TRAVERSAL by default.
  panel_ = new wxPanel(this);
  //panel_->SetBackgroundColour(*wxWHITE);  // TODO

  //----------------------------------------------------------------------------

  regex_toggle_button_ = NewToggleButton(ID_REGEX_TOGGLE_BUTTON,
                                         wxT("fw_regex"),
                                         kTrUseRegex);

  case_toggle_button_ = NewToggleButton(ID_CASE_TOGGLE_BUTTON,
                                        wxT("fw_case"),
                                        kTrCaseSensitive);

  word_toggle_button_ = NewToggleButton(ID_WORD_TOGGLE_BUTTON,
                                        wxT("fw_word"),
                                        kTrMatchWholeWord);

  regex_toggle_button_->set_toggle(GetBit(flags_, kFind_UseRegex));
  case_toggle_button_->set_toggle(GetBit(flags_, kFind_CaseSensitive));
  word_toggle_button_->set_toggle(GetBit(flags_, kFind_MatchWholeWord));

  //----------------------------------------------------------------------------

  mode_toggle_button_ = NewToggleButton(ID_FIND_MODE_TOGGLE_BUTTON,
                                        wxT("fw_mode"),
                                        kTrSwitchMode);

  //----------------------------------------------------------------------------

  find_combobox_ = new wxComboBox(panel_, ID_FIND_COMBOBOX);

  // Initialize find combobox with find history.
  const std::list<wxString>& find_strings = session_->find_strings();
  {
    std::list<wxString>::const_iterator it = find_strings.begin();
    for (; it != find_strings.end(); ++it) {
      find_combobox_->Append(*it);
    }
  }

  if (!find_combobox_->IsListEmpty()) {
    find_combobox_->Select(0);
  }

  //----------------------------------------------------------------------------

  replace_combobox_ = new wxComboBox(panel_, ID_REPLACE_COMBOBOX);

  // Initialize replace combobox with replace history.
  const std::list<wxString>& replace_strings = session_->replace_strings();
  {
    std::list<wxString>::const_iterator it = replace_strings.begin();
    for (; it != replace_strings.end(); ++it) {
      replace_combobox_->Append(*it);
    }
  }

  if (!replace_combobox_->IsListEmpty()) {
    replace_combobox_->Select(0);
  }

  //----------------------------------------------------------------------------

  find_button_ = NewButton(ID_FIND_BUTTON, wxT("fw_find"), kTrFind);
  find_all_button_ = NewButton(ID_FIND_ALL_BUTTON,
                               wxT("fw_find_all"),
                               kTrFindAll);

  replace_button_ = NewButton(ID_REPLACE_BUTTON, wxT("fw_replace"), kTrReplace);
  replace_all_button_ = NewButton(ID_REPLACE_ALL_BUTTON,
                                  wxT("fw_replace_all"),
                                  kTrReplaceAll);

  find_button_->SetDefault();  // Set default for ENTER key.

  // Layout controls.

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);

  wxSizer* option_hsizer = new wxBoxSizer(wxHORIZONTAL);
  option_hsizer->Add(regex_toggle_button_, 0, 0, 0);
  option_hsizer->Add(case_toggle_button_, 0, wxLEFT, kPadding);
  option_hsizer->Add(word_toggle_button_, 0, wxLEFT, kPadding);
  option_hsizer->AddStretchSpacer();
  option_hsizer->Add(mode_toggle_button_, 0, 0);
  vsizer->Add(option_hsizer, 0, wxEXPAND | wxALL, kPadding);

  wxSizer* find_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_hsizer->Add(find_combobox_, 1, wxALIGN_CENTER_VERTICAL);
  find_hsizer->Add(find_button_,
                   0,
                   wxALIGN_CENTER_VERTICAL | wxLEFT,
                   kPadding);
  find_hsizer->Add(find_all_button_,
                   0,
                   wxALIGN_CENTER_VERTICAL | wxLEFT,
                   kPadding);
  vsizer->Add(find_hsizer, 0, wxEXPAND | wxALL, kPadding);

  wxSizer* replace_hsizer = new wxBoxSizer(wxHORIZONTAL);
  replace_hsizer->Add(replace_combobox_, 1, wxALIGN_CENTER_VERTICAL);
  replace_hsizer->Add(replace_button_,
                      0,
                      wxALIGN_CENTER_VERTICAL | wxLEFT,
                      kPadding);
  replace_hsizer->Add(replace_all_button_,
                      0,
                      wxALIGN_CENTER_VERTICAL | wxLEFT,
                      kPadding);
  vsizer->Add(replace_hsizer, 0, wxEXPAND | wxALL, kPadding);

  panel_->SetSizer(vsizer);

  if (mode_ == kFindMode) {
    mode_toggle_button_->set_toggle(false);
    LayoutAsFind();
  } else {
    mode_toggle_button_->set_toggle(true);
    LayoutAsReplace();
  }

  return true;
}

FindWindow::~FindWindow() {
}

void FindWindow::UpdateLayout() {
  if (mode_ == kFindMode) {
    mode_toggle_button_->set_toggle(false);
    LayoutAsFind();
  } else {
    mode_toggle_button_->set_toggle(true);
    LayoutAsReplace();
  }
}

void FindWindow::SetFindString(const wxString& find_string) {
  find_combobox_->SetValue(find_string);
  AddFindString(find_string);
}

// Update control values on activate.
void FindWindow::OnActivate(wxActivateEvent& evt) {
  // Focus and select the text so the user can change it directly.
  find_combobox_->SetFocus();
  find_combobox_->SelectAll();
}

void FindWindow::OnClose(wxCloseEvent& evt) {
  session_->set_find_window_rect(GetScreenRect());
  session_->set_find_flags(flags_);

  evt.Skip();
}

void FindWindow::OnKeyDownHook(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_ESCAPE) {
    Close();
  } else {
    evt.Skip();
  }
}

void FindWindow::OnUseRegexToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_UseRegex, evt.IsChecked());
}

void FindWindow::OnCaseSensitiveToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_CaseSensitive, evt.IsChecked());
}

void FindWindow::OnMatchWholeWordToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_MatchWholeWord, evt.IsChecked());
}

void FindWindow::OnSearchReverselyToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_Reversely, evt.IsChecked());
}

void FindWindow::OnModeToggle(wxCommandEvent& evt) {
  if (evt.IsChecked()) {
    LayoutAsReplace();
    mode_ = kReplaceMode;
  } else {
    LayoutAsFind();
    mode_ = kFindMode;
  }
}

void FindWindow::OnFind(wxCommandEvent& evt) {
  wxString str = find_combobox_->GetValue();
  if (str.IsEmpty()) {
    return;
  }

  AddFindString(str);

  book_frame_->FindInActivePage(str.ToStdWstring(), flags_);
}

void FindWindow::OnFindAll(wxCommandEvent& evt) {
  wxString str = find_combobox_->GetValue();
  if (str.IsEmpty()) {
    return;
  }

  AddFindString(str);

  book_frame_->FindAllInActivePage(str.ToStdWstring(), flags_);
}

void FindWindow::OnReplace(wxCommandEvent& evt) {
  wxString str = find_combobox_->GetValue();
  if (str.IsEmpty()) {
    return;
  }

  AddFindString(str);

  wxString replace_str = replace_combobox_->GetValue();

  if (!replace_str.IsEmpty()) {
    AddReplaceString(replace_str);
  }

  book_frame_->ReplaceInActivePage(str.ToStdWstring(),
                                   replace_str.ToStdWstring(),
                                   flags_);
}

void FindWindow::OnReplaceAll(wxCommandEvent& evt) {
  wxString str = find_combobox_->GetValue();
  if (str.IsEmpty()) {
    return;
  }

  AddFindString(str);

  wxString replace_str = replace_combobox_->GetValue();

  if (!replace_str.IsEmpty()) {
    AddReplaceString(replace_str);
  }

  book_frame_->ReplaceAllInActivePage(str.ToStdWstring(),
                                      replace_str.ToStdWstring(),
                                      flags_);
}

void FindWindow::AddFindString(const wxString& string) {
  if (session_->AddFindString(string)) {
    // The find string is new. Simply push to find combobox at front.
    find_combobox_->Insert(string, 0);
  } else {
    // Move the find string to front in the find combobox.
    int old_index = find_combobox_->FindString(string, true);
    if (old_index != wxNOT_FOUND) {
      find_combobox_->Delete(old_index);
    }
    find_combobox_->Insert(string, 0);
    find_combobox_->Select(0);
  }
}

void FindWindow::AddReplaceString(const wxString& string) {
  if (session_->AddReplaceString(string)) {
    // The replace string is new. Simply push to replace combobox at front.
    replace_combobox_->Insert(string, 0);
  } else {
    // Move the find string to front in the replace combobox.
    int old_index = replace_combobox_->FindString(string, true);
    if (old_index != wxNOT_FOUND) {
      replace_combobox_->Delete(old_index);
    }
    replace_combobox_->Insert(string, 0);
    replace_combobox_->Select(0);
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

void FindWindow::LayoutAsFind() {
  panel_->GetSizer()->Show(kReplaceSizerIndex, false);
  UpdateSizes();
  SetTitle(kTrFind);
}

void FindWindow::LayoutAsReplace() {
  panel_->GetSizer()->Show(kReplaceSizerIndex, true);
  UpdateSizes();
  SetTitle(kTrReplace);
}

ui::BitmapToggleButton* FindWindow::NewToggleButton(wxWindowID id,
                                                    const wxString& bitmap,
                                                    const wxString& tooltip) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(panel_, id);
  button->SetBitmaps(skin::GetIcon(bitmap),
                     skin::GetIcon(bitmap + wxT("_toggle")));
  button->SetToolTip(tooltip);
  return button;
}

wxBitmapButton* FindWindow::NewButton(int id,
                                      const wxString& bitmap,
                                      const wxString& tooltip) {
  wxBitmapButton* button = new wxBitmapButton(panel_,
                                              id,
                                              skin::GetIcon(bitmap));
  button->SetToolTip(tooltip);
  return button;
}

}  // namespace jil
