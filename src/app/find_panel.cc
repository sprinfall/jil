#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/log.h"

#include "editor/text_buffer.h"

#include "app/bitmap_toggle_button.h"
#include "app/session.h"
#include "app/text_page.h"
#include "app/book_frame.h"
#include "app/separator.h"
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

const int kPadding = 5;
const int kReplaceSizerIndex = 1;

// Control ID's.
enum {
  kCaseSensitiveToggleId = wxID_HIGHEST + 100,
  kMatchWholeWordToggleId,
  kSearchReverselyToggleId,
  kUseRegexToggleId,

  kModeToggleId,

  kFindComboBoxId,
  kReplaceComboBoxId,
  kLocationComboBoxId,

  kFindButtonId,
  kFindAllButtonId,
  kReplaceButtonId,
  kReplaceAllButtonId,
};

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
//EVT_ACTIVATE(FindPanel::OnActivate)
//EVT_CLOSE(FindPanel::OnClose)
//EVT_CHAR_HOOK(FindPanel::OnKeyDownHook)
EVT_TOGGLEBUTTON(kUseRegexToggleId, FindPanel::OnUseRegexToggle)
EVT_TOGGLEBUTTON(kCaseSensitiveToggleId, FindPanel::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON(kMatchWholeWordToggleId, FindPanel::OnMatchWholeWordToggle)
EVT_TOGGLEBUTTON(kSearchReverselyToggleId, FindPanel::OnSearchReverselyToggle)
EVT_TOGGLEBUTTON(kModeToggleId, FindPanel::OnModeToggle)
EVT_BUTTON(kFindButtonId, FindPanel::OnFind)
EVT_BUTTON(kFindAllButtonId, FindPanel::OnFindAll)
EVT_BUTTON(kReplaceButtonId, FindPanel::OnReplace)
EVT_BUTTON(kReplaceAllButtonId, FindPanel::OnReplaceAll)
END_EVENT_TABLE();

FindPanel::FindPanel()
    : session_(NULL), mode_(kFindMode), book_frame_(NULL) {
}

FindPanel::FindPanel(Session* session, int mode)
    : session_(session), mode_(mode), book_frame_(NULL) {
}

bool FindPanel::Create(BookFrame* book_frame, wxWindowID id) {
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();

  book_frame_ = book_frame;

  // Note: Window title will be set later according to the mode.
  if (!wxPanel::Create(book_frame, id)) {
    return false;
  }

  //------------------------------------

  top_border_ = new Separator(this);
  // TODO: theme
  top_border_->SetColor(wxColour(125, 125, 125));

  //------------------------------------

  use_regex_toggle_ = new BitmapToggleButton(this, kUseRegexToggleId);
  use_regex_toggle_->SetBitmaps(skin::GetIcon(wxT("fw_regex")),
                                skin::GetIcon(wxT("fw_regex_toggle")));
  use_regex_toggle_->SetToolTip(kTrUseRegex);

  case_sensitive_toggle_ = new BitmapToggleButton(this,
                                                  kCaseSensitiveToggleId);
  case_sensitive_toggle_->SetBitmaps(skin::GetIcon(wxT("fw_case")),
                                     skin::GetIcon(wxT("fw_case_toggle")));
  case_sensitive_toggle_->SetToolTip(kTrCaseSensitive);

  match_whole_word_toggle_ = new BitmapToggleButton(this,
                                                    kMatchWholeWordToggleId);
  match_whole_word_toggle_->SetBitmaps(
      skin::GetIcon(wxT("fw_whole_word")),
      skin::GetIcon(wxT("fw_whole_word_toggle")));
  match_whole_word_toggle_->SetToolTip(kTrMatchWholeWord);

  search_reversely_toggle_ = new BitmapToggleButton(this,
                                                    kSearchReverselyToggleId);
  search_reversely_toggle_->SetBitmaps(
      skin::GetIcon(wxT("fw_reversely")),
      skin::GetIcon(wxT("fw_reversely_toggle")),
      skin::GetIcon(wxT("fw_reversely_disabled")),
      skin::GetIcon(wxT("fw_reversely_toggle_disabled")));
  search_reversely_toggle_->SetToolTip(kTrSearchReversely);

  // Initialize the toggle states.
  use_regex_toggle_->set_toggle(GetBit(flags_, kFindUseRegex));
  case_sensitive_toggle_->set_toggle(GetBit(flags_, kFindCaseSensitive));
  match_whole_word_toggle_->set_toggle(GetBit(flags_, kFindMatchWholeWord));
  search_reversely_toggle_->set_toggle(GetBit(flags_, kFindReversely));
  if (GetBit(flags_, kFindUseRegex)) {
    search_reversely_toggle_->Enable(false);
  }

  //------------------------------------

  mode_toggle_ = new BitmapToggleButton(this, kModeToggleId);
  mode_toggle_->SetToolTip(kTrSwitchMode);
  mode_toggle_->SetBitmaps(skin::GetIcon(wxT("fw_mode")),
                           skin::GetIcon(wxT("fw_mode_toggle")));

  mode_toggle_->Hide(); // TODO

  //------------------------------------

  find_combobox_ = new wxComboBox(this, kFindComboBoxId);

  // Initialize find combobox with find history.
  const std::list<wxString>& find_strings = session_->recent_find_strings();
  {
    std::list<wxString>::const_iterator it = find_strings.begin();
    for (; it != find_strings.end(); ++it) {
      find_combobox_->Append(*it);
    }
  }

  if (!find_combobox_->IsListEmpty()) {
    find_combobox_->Select(0);
  }

  //------------------------------------

  replace_combobox_ = new wxComboBox(this, kReplaceComboBoxId);

  // Initialize replace combobox with replace history.
  const std::list<wxString>& replace_strings = session_->recent_replace_strings();
  {
    std::list<wxString>::const_iterator it = replace_strings.begin();
    for (; it != replace_strings.end(); ++it) {
      replace_combobox_->Append(*it);
    }
  }

  if (!replace_combobox_->IsListEmpty()) {
    replace_combobox_->Select(0);
  }

  //------------------------------------

  find_button_ = new wxButton(this, kFindButtonId, kTrFind);
  find_all_button_ = new wxButton(this, kFindAllButtonId, kTrFindAll);
  replace_button_ = new wxButton(this, kReplaceButtonId, kTrReplace);
  replace_all_button_ = new wxButton(this, kReplaceAllButtonId, kTrReplaceAll);

  find_button_->SetDefault();  // Set default for ENTER key.

  //------------------------------------

  if (mode_ == kFindMode) {
    mode_toggle_->set_toggle(false);
    LayoutAsFind();
  } else {
    mode_toggle_->set_toggle(true);
    LayoutAsReplace();
  }

  return true;
}

FindPanel::~FindPanel() {
}

void FindPanel::UpdateLayout() {
  if (mode_ == kFindMode) {
    mode_toggle_->set_toggle(false);
    LayoutAsFind();
  } else {
    mode_toggle_->set_toggle(true);
    LayoutAsReplace();
  }
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_combobox_->SetValue(find_string);
  AddFindString(find_string);
}

//// Update control values on activate.
//void FindPanel::OnActivate(wxActivateEvent& evt) {
//  // Focus and select the text so the user can change it directly.
//  find_combobox_->SetFocus();
//  find_combobox_->SelectAll();
//}

//void FindPanel::OnClose(wxCloseEvent& evt) {
//  session_->set_find_window_rect(GetScreenRect());
//  session_->set_find_flags(flags_);
//
//  evt.Skip();
//}

//void FindPanel::OnKeyDownHook(wxKeyEvent& evt) {
//  if (evt.GetKeyCode() == WXK_ESCAPE) {
//    Close();
//  } else {
//    evt.Skip();
//  }
//}

void FindPanel::OnUseRegexToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindUseRegex, evt.IsChecked());
  // Not supported yet.
  search_reversely_toggle_->Enable(!evt.IsChecked());
}

void FindPanel::OnCaseSensitiveToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindCaseSensitive, evt.IsChecked());
}

void FindPanel::OnMatchWholeWordToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindMatchWholeWord, evt.IsChecked());
}

void FindPanel::OnSearchReverselyToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFindReversely, evt.IsChecked());
}

void FindPanel::OnModeToggle(wxCommandEvent& evt) {
  if (evt.IsChecked()) {
    LayoutAsReplace();
    mode_ = kReplaceMode;
  } else {
    LayoutAsFind();
    mode_ = kFindMode;
  }
}

void FindPanel::OnFind(wxCommandEvent& evt) {
  wxString str = find_combobox_->GetValue();
  if (str.IsEmpty()) {
    return;
  }

  AddFindString(str);

  book_frame_->FindInActivePage(str.ToStdWstring(), flags_);
}

void FindPanel::OnFindAll(wxCommandEvent& evt) {
  wxString str = find_combobox_->GetValue();
  if (str.IsEmpty()) {
    return;
  }

  AddFindString(str);

  book_frame_->FindAllInActivePage(str.ToStdWstring(), flags_);
}

void FindPanel::OnReplace(wxCommandEvent& evt) {
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

void FindPanel::OnReplaceAll(wxCommandEvent& evt) {
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

void FindPanel::AddFindString(const wxString& string) {
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

void FindPanel::AddReplaceString(const wxString& string) {
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

void FindPanel::LayoutAsFind() {
  replace_combobox_->Hide();
  replace_button_->Hide();
  replace_all_button_->Hide();

  int flags = wxALIGN_CENTER_VERTICAL | wxLEFT;

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);

  ctrl_hsizer->Add(use_regex_toggle_, 0, flags, 0);
  ctrl_hsizer->Add(case_sensitive_toggle_, 0, flags, kPadding);
  ctrl_hsizer->Add(match_whole_word_toggle_, 0, flags, kPadding);
  ctrl_hsizer->Add(search_reversely_toggle_, 0, flags, kPadding);
  ctrl_hsizer->Add(find_combobox_, 1, flags, kPadding);
  ctrl_hsizer->Add(find_button_, 0, flags, kPadding);
  ctrl_hsizer->Add(find_all_button_, 0, flags, kPadding);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(top_border_, 0, wxEXPAND);
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
  SetSizer(vsizer);

  Layout();
}

void FindPanel::LayoutAsReplace() {
  replace_combobox_->Show();
  replace_button_->Show();
  replace_all_button_->Show();

  int flags = wxALIGN_CENTER_VERTICAL | wxLEFT;

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);

  wxSizer* flag_vsizer = new wxBoxSizer(wxVERTICAL);

  wxSizer* find_flag_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_flag_hsizer->Add(use_regex_toggle_, 0, flags, 0);
  find_flag_hsizer->Add(case_sensitive_toggle_, 0, flags, kPadding);
  find_flag_hsizer->Add(match_whole_word_toggle_, 0, flags, kPadding);
  find_flag_hsizer->Add(search_reversely_toggle_, 0, flags, kPadding);

  flag_vsizer->Add(find_flag_hsizer, 0, wxEXPAND);

  wxSizer* find_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_hsizer->Add(find_combobox_, 1, flags, kPadding);
  find_hsizer->Add(find_button_, 0, flags, kPadding);
  find_hsizer->Add(find_all_button_, 0, flags, kPadding);

  wxSizer* replace_hsizer = new wxBoxSizer(wxHORIZONTAL);
  replace_hsizer->Add(replace_combobox_, 1, flags, kPadding);
  replace_hsizer->Add(replace_button_, 0, flags, kPadding);
  replace_hsizer->Add(replace_all_button_, 0, flags, kPadding);

  ctrl_hsizer->Add(flag_vsizer, 0, wxEXPAND);

  wxSizer* body_vsizer = new wxBoxSizer(wxVERTICAL);
  body_vsizer->Add(find_hsizer, 0, wxEXPAND);
  body_vsizer->AddSpacer(kPadding);
  body_vsizer->Add(replace_hsizer, 0, wxEXPAND);

  ctrl_hsizer->Add(body_vsizer, 1, wxEXPAND);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(top_border_, 0, wxEXPAND);
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
  SetSizer(vsizer);

  Layout();
}

}  // namespace jil
