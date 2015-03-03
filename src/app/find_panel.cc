#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/sizer.h"
#include "wx/log.h"

#include "ui/bitmap_toggle_button.h"
#include "ui/color.h"
#include "ui/text_button.h"

#include "editor/text_buffer.h"

#include "app/book_frame.h"
#include "app/session.h"
#include "app/skin.h"
#include "app/text_page.h"
#include "app/util.h"

#define kTrUseRegex _("Use regular expression")
#define kTrCaseSensitive _("Case sensitive")
#define kTrMatchWholeWord _("Match whole word")
#define kTrSearchReversely _("Search reversely")

#define kTrFind _("Find")
#define kTrReplace _("Replace")
#define kTrFindAll _("Find All")
#define kTrReplaceAll _("Replace All")

namespace jil {

const int kPadding = 5;

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
EVT_PAINT(FindPanel::OnPaint)
EVT_TOGGLEBUTTON(kUseRegexToggleId, FindPanel::OnUseRegexToggle)
EVT_TOGGLEBUTTON(kCaseSensitiveToggleId, FindPanel::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON(kMatchWholeWordToggleId, FindPanel::OnMatchWholeWordToggle)
EVT_TOGGLEBUTTON(kSearchReverselyToggleId, FindPanel::OnSearchReverselyToggle)
EVT_TOGGLEBUTTON(kModeToggleId, FindPanel::OnModeToggle)
EVT_BUTTON(kFindButtonId, FindPanel::OnFind)
EVT_BUTTON(kFindAllButtonId, FindPanel::OnFindAll)
EVT_BUTTON(kReplaceButtonId, FindPanel::OnReplace)
EVT_BUTTON(kReplaceAllButtonId, FindPanel::OnReplaceAll)
END_EVENT_TABLE()

FindPanel::FindPanel()
    : session_(NULL), mode_(kFindMode), book_frame_(NULL) {
}

FindPanel::FindPanel(Session* session, int mode)
    : session_(session), mode_(mode), book_frame_(NULL) {
}

bool FindPanel::Create(BookFrame* book_frame, wxWindowID id) {
  assert(theme_);
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();

  book_frame_ = book_frame;

  if (!wxPanel::Create(book_frame, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(theme_->GetColor(BG));

  // Create text button style.
  InitButtonStyle();

  //------------------------------------

  use_regex_toggle_ = CreateToggleButton(kUseRegexToggleId, wxT("find_regex"));
  use_regex_toggle_->SetToolTip(kTrUseRegex);

  case_sensitive_toggle_ = CreateToggleButton(kCaseSensitiveToggleId,
                                              wxT("find_case"));
  case_sensitive_toggle_->SetToolTip(kTrCaseSensitive);

  match_whole_word_toggle_ = CreateToggleButton(kMatchWholeWordToggleId,
                                                wxT("find_whole_word"));
  match_whole_word_toggle_->SetToolTip(kTrMatchWholeWord);

  search_reversely_toggle_ = CreateToggleButton(kSearchReverselyToggleId,
                                                wxT("find_reversely"));
  search_reversely_toggle_->SetToolTip(kTrSearchReversely);

  // Initialize the toggle button states.
  use_regex_toggle_->set_toggle(GetBit(flags_, kFindUseRegex));
  case_sensitive_toggle_->set_toggle(GetBit(flags_, kFindCaseSensitive));
  match_whole_word_toggle_->set_toggle(GetBit(flags_, kFindMatchWholeWord));
  search_reversely_toggle_->set_toggle(GetBit(flags_, kFindReversely));

  if (GetBit(flags_, kFindUseRegex)) {
    search_reversely_toggle_->Enable(false);
  }

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

  find_button_ = CreateTextButton(kFindButtonId, kTrFind);
  find_all_button_ = CreateTextButton(kFindAllButtonId, kTrFindAll);
  replace_button_ = CreateTextButton(kReplaceButtonId, kTrReplace);
  replace_all_button_ = CreateTextButton(kReplaceAllButtonId, kTrReplaceAll);

  //find_button_->SetDefault();  // Set default for ENTER key.

  //------------------------------------

  if (mode_ == kFindMode) {
    LayoutAsFind();
  } else {
    LayoutAsReplace();
  }

  return true;
}

FindPanel::~FindPanel() {
}

bool FindPanel::Destroy() {
  session_->set_find_flags(flags_);
  return wxPanel::Destroy();
}

void FindPanel::UpdateLayout() {
  if (mode_ == kFindMode) {
    LayoutAsFind();
  } else {
    LayoutAsReplace();
  }
}

void FindPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  wxRect rect = GetClientRect();

  dc.SetPen(wxPen(theme_->GetColor(BORDER)));
  dc.DrawLine(rect.x, rect.y, rect.GetRight() + 1, rect.GetTop());
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_combobox_->SetValue(find_string);
  AddFindString(find_string);
}

void FindPanel::SetFocus() {
  wxPanel::SetFocus();

  find_combobox_->SetFocus();
  if (!find_combobox_->GetValue().IsEmpty()) {
    find_combobox_->SelectAll();
  }
}

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
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
  SetSizer(vsizer);

  Layout();
}

void FindPanel::InitButtonStyle() {
  button_style_.reset(new ui::ButtonStyle);

  editor::SharedTheme button_theme = theme_->GetTheme(BUTTON);
  if (button_theme.get() == NULL) {
    return;
  }

  for (int part = 0; part < ui::ButtonStyle::PARTS; ++part) {
    editor::SharedTheme part_theme = button_theme->GetTheme(part);
    if (part_theme) {
      for (int state = 0; state < ui::ButtonStyle::STATES; ++state) {
        button_style_->SetColor(part, state, part_theme->GetColor(state));
      }
    }
  }
}

ui::BitmapToggleButton* FindPanel::CreateToggleButton(int id, const wxString& bitmap) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(button_style_);
  button->Create(this, id);
  button->SetBitmap(skin::GetIcon(bitmap));
  button->set_user_best_size(wxSize(24, 24));
  return button;
}

ui::TextButton* FindPanel::CreateTextButton(int id, const wxString& label) {
  ui::TextButton* button = new ui::TextButton(button_style_);
  button->Create(this, id, label);
  button->SetMinSize(wxSize(80, -1));
  return button;
}

}  // namespace jil
