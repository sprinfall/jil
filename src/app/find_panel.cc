#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/combobox.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"

#include "ui/bitmap_toggle_button.h"
#include "ui/color.h"
#include "ui/text_button.h"

#include "editor/text_buffer.h"

#include "app/id.h"
#include "app/session.h"
#include "app/skin.h"
#include "app/text_page.h"
#include "app/util.h"

#define kTrCurrentPage    _("Current Page")
#define kTrAllPages       _("All Pages")
#define kTrFolders        _("Folders")

#define kTrLocation       _("Show location panel")
#define kTrUseRegex       _("Use regular expression")
#define kTrCaseSensitive  _("Case sensitive")
#define kTrMatchWord      _("Match whole word")

#define kTrFind           _("Find")
#define kTrReplace        _("Replace")
#define kTrFindAll        _("Find All")
#define kTrReplaceAll     _("Replace All")

namespace jil {

const int kPadding = 5;

DEFINE_EVENT_TYPE(kFindPanelEvent)

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
EVT_PAINT(FindPanel::OnPaint)

EVT_COMBOBOX(ID_LOCATION_COMBOBOX, FindPanel::OnLocationComboBox)
//EVT_MENU_RANGE(ID_MENU_LOCATION_BEGIN, ID_MENU_LOCATION_END - 1, FindPanel::OnLocationMenu)

EVT_TOGGLEBUTTON(ID_LOCATION_TOGGLE_BUTTON, FindPanel::OnLocationToggle)
EVT_TOGGLEBUTTON(ID_USE_REGEX_TOGGLE_BUTTON, FindPanel::OnUseRegexToggle)
EVT_TOGGLEBUTTON(ID_CASE_SENSITIVE_TOGGLE_BUTTON, FindPanel::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON(ID_MATCH_WORD_TOGGLE_BUTTON, FindPanel::OnMatchWordToggle)

EVT_BUTTON(ID_FIND_BUTTON, FindPanel::OnFind)
EVT_BUTTON(ID_FIND_ALL_BUTTON, FindPanel::OnFindAll)
EVT_BUTTON(ID_REPLACE_BUTTON, FindPanel::OnReplace)
EVT_BUTTON(ID_REPLACE_ALL_BUTTON, FindPanel::OnReplaceAll)

EVT_TEXT(ID_FIND_COMBOBOX, FindPanel::OnFindText)
EVT_TEXT_ENTER(ID_FIND_COMBOBOX, FindPanel::OnFindTextEnter)
END_EVENT_TABLE()

FindPanel::FindPanel()
    : session_(NULL)
    , mode_(kFindMode)
    , flags_(0)
    , location_(kCurrentPage)
    , show_location_(false) {
}

FindPanel::FindPanel(Session* session, int mode)
    : session_(session)
    , mode_(mode)
    , flags_(0)
    , location_(kCurrentPage)
    , show_location_(false) {
}

bool FindPanel::Create(wxWindow* parent, wxWindowID id) {
  assert(theme_);
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();
  //location_ = session_->find_location();

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(theme_->GetColor(BG_TOP));

  InitComboStyle();
  InitButtonStyle();

  //----------------------------------------------------------------------------

  location_combobox_ = new ui::ComboBox(combo_style_);
  location_combobox_->Create(this, ID_LOCATION_COMBOBOX, wxEmptyString);
  location_combobox_->SetForegroundColour(*wxWHITE);  // TODO: Get fg from theme
  location_combobox_->Hide();

  location_combobox_->Append(kTrCurrentPage);
  location_combobox_->Append(kTrAllPages);
  location_combobox_->Append(kTrFolders);

  folders_text_ctrl_ = new wxTextCtrl(this, ID_FOLDERS_TEXTCTRL);
  folders_text_ctrl_->Hide();

  //----------------------------------------------------------------------------

  location_toggle_button_ = NewToggleButton(ID_LOCATION_TOGGLE_BUTTON, wxT("location"));
  location_toggle_button_->SetToolTip(kTrLocation);

  use_regex_toggle_button_ = NewToggleButton(ID_USE_REGEX_TOGGLE_BUTTON, wxT("use_regex"));
  use_regex_toggle_button_->SetToolTip(kTrUseRegex);

  case_sensitive_toggle_button_ = NewToggleButton(ID_CASE_SENSITIVE_TOGGLE_BUTTON, wxT("case_sensitive"));
  case_sensitive_toggle_button_->SetToolTip(kTrCaseSensitive);

  match_word_toggle_button_ = NewToggleButton(ID_MATCH_WORD_TOGGLE_BUTTON, wxT("match_word"));
  match_word_toggle_button_->SetToolTip(kTrMatchWord);

  // Initialize toggle button states.
  use_regex_toggle_button_->set_toggle(GetBit(flags_, kFind_UseRegex));
  case_sensitive_toggle_button_->set_toggle(GetBit(flags_, kFind_CaseSensitive));
  match_word_toggle_button_->set_toggle(GetBit(flags_, kFind_MatchWord));

  //------------------------------------

  find_combobox_ = new wxComboBox(this, ID_FIND_COMBOBOX);
  find_combobox_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);

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

  //------------------------------------

  replace_combobox_ = new wxComboBox(this, ID_REPLACE_COMBOBOX);

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

  //------------------------------------

  find_button_ = NewTextButton(ID_FIND_BUTTON, kTrFind);
  find_all_button_ = NewTextButton(ID_FIND_ALL_BUTTON, kTrFindAll);
  replace_button_ = NewTextButton(ID_REPLACE_BUTTON, kTrReplace);
  replace_all_button_ = NewTextButton(ID_REPLACE_ALL_BUTTON, kTrReplaceAll);

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

void FindPanel::SetLocation(FindLocation location) {
  location_ = location;

  if (location == kCurrentPage) {
    location_combobox_->SetLabel(kTrCurrentPage);
  } else if (location == kAllPages) {
    location_combobox_->SetLabel(kTrAllPages);
  } else if (location == kFolders) {
    location_combobox_->SetLabel(kTrFolders);
  }

  if (!show_location_) {
    return;
  }

  location_combobox_->Refresh();

  if (location == kFolders) {
    folders_text_ctrl_->Show();
  } else {
    folders_text_ctrl_->Hide();
  }

  UpdateLayout();
}

void FindPanel::UpdateLayout() {
  if (mode_ == kFindMode) {
    LayoutAsFind();
  } else {
    LayoutAsReplace();
  }
}

void FindPanel::SetFocus() {
  wxPanel::SetFocus();

  find_combobox_->SetFocus();
  if (!find_combobox_->GetValue().IsEmpty()) {
    find_combobox_->SelectAll();
  }
}

void FindPanel::OnLocationComboBox(wxCommandEvent& evt) {
  int index = evt.GetSelection();
  if (index < 0 || index >= kFindLocations) {
    return;
  }
  SetLocation(static_cast<FindLocation>(index));

  //wxMenu menu;
  //menu.AppendCheckItem(ID_MENU_CURRENT_PAGE, kTrCurrentPage);
  //menu.AppendCheckItem(ID_MENU_ALL_PAGES, kTrAllPages);
  //menu.AppendCheckItem(ID_MENU_FOLDERS, kTrFolders);

  //menu.Check(ID_MENU_LOCATION_BEGIN + location_, true);

  //PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void FindPanel::OnLocationMenu(wxCommandEvent& evt) {
  int index = evt.GetId() - ID_MENU_LOCATION_BEGIN;
  if (index < 0 || index >= kFindLocations) {
    return;
  }
  SetLocation(static_cast<FindLocation>(index));
}

void FindPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  wxRect rect = GetClientRect();
  wxRect update_rect = GetUpdateClientRect();

  // Background
  wxRect bg_rect(update_rect.x, 0, update_rect.width, 0);
  bg_rect.y = rect.y + 2;
  bg_rect.height = rect.height - 2;
  wxColour bg_top = theme_->GetColor(BG_TOP);
  wxColour bg_bottom = theme_->GetColor(BG_BOTTOM);
  dc.GradientFillLinear(bg_rect, bg_bottom, bg_top, wxNORTH);

  // Borders
  int border_y = rect.y;
  dc.SetPen(wxPen(theme_->GetColor(BORDER_OUTER)));
  dc.DrawLine(bg_rect.x, border_y, bg_rect.GetRight() + 1, border_y);
  ++border_y;
  dc.SetPen(wxPen(theme_->GetColor(BORDER_INNER)));
  dc.DrawLine(bg_rect.x, border_y, bg_rect.GetRight() + 1, border_y);
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_combobox_->SetValue(find_string);
  find_combobox_->SelectAll();
  AddFindString(find_string);
}

void FindPanel::OnLocationToggle(wxCommandEvent& evt) {
  show_location_ = evt.IsChecked();
  PostEvent(kLayoutEvent, wxEmptyString, wxEmptyString);
  UpdateLayout();
}

void FindPanel::OnUseRegexToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_UseRegex, evt.IsChecked());
}

void FindPanel::OnCaseSensitiveToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_CaseSensitive, evt.IsChecked());
}

void FindPanel::OnMatchWordToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_MatchWord, evt.IsChecked());
}

void FindPanel::OnFind(wxCommandEvent& evt) {
  HandleFind(false);
}

void FindPanel::OnFindAll(wxCommandEvent& evt) {
  HandleFind(true);
}

void FindPanel::OnReplace(wxCommandEvent& evt) {
  HandleReplace(false);
}

void FindPanel::OnReplaceAll(wxCommandEvent& evt) {
  HandleReplace(true);
}

void FindPanel::OnFindText(wxCommandEvent& evt) {
  wxString find_str = find_combobox_->GetValue();
  // Post event even if the find string is empty so that the previous matching
  // results can be cleared.
  PostEvent(kFindStrEvent, find_str, wxEmptyString);
}

void FindPanel::OnFindTextEnter(wxCommandEvent& evt) {
  HandleFind(false);
}

void FindPanel::HandleFind(bool all) {
  wxString find_str = find_combobox_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    int event_type = all ? kFindAllEvent : kFindEvent;
    PostEvent(event_type, find_str, wxEmptyString);
  }
}

void FindPanel::HandleReplace(bool all) {
  wxString find_str = find_combobox_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    wxString replace_str = replace_combobox_->GetValue();
    if (!replace_str.IsEmpty()) {
      AddReplaceString(replace_str);
    }

    int event_type = all ? kReplaceAllEvent : kReplaceEvent;
    PostEvent(kReplaceAllEvent, find_str, replace_str);
  }
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
  Freeze();

  ShowReplace(false);
  ShowLocation(show_location_);

  if (show_location_) {
    CommonLayout(true, false);
  } else {
    wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);

    AddToggleButtons(ctrl_hsizer);

    ctrl_hsizer->Add(find_combobox_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
    ctrl_hsizer->Add(find_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
    ctrl_hsizer->Add(find_all_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);

    wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    vsizer->AddSpacer(2);  // Top borders
    vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
    SetSizer(vsizer);

    Layout();
  }

  Thaw();
}

void FindPanel::LayoutAsReplace() {
  Freeze();

  ShowReplace(true);
  ShowLocation(show_location_);

  if (show_location_) {
    CommonLayout(true, true);
  } else {
    CommonLayout(false, true);
  }

  Thaw();
}

void FindPanel::CommonLayout(bool with_location, bool with_replace) {
  wxSizer* head_vsizer = new wxBoxSizer(wxVERTICAL);
  {
    if (with_location) {
      wxSizer* location_head_hsizer = new wxBoxSizer(wxHORIZONTAL);
      location_head_hsizer->Add(location_combobox_, 1, wxALIGN_CENTER_VERTICAL);

      head_vsizer->Add(location_head_hsizer, 1, wxEXPAND);
      head_vsizer->AddSpacer(2);
    }
    
    wxSizer* find_head_hsizer = new wxBoxSizer(wxHORIZONTAL);
    AddToggleButtons(find_head_hsizer);
    head_vsizer->Add(find_head_hsizer, 1, wxEXPAND);

    if (with_replace) {
      head_vsizer->AddSpacer(2);
      head_vsizer->AddStretchSpacer(1);
    }
  }

  wxSizer* body_vsizer = new wxBoxSizer(wxVERTICAL);
  {
    if (with_location) {
      if (location_ == kFolders) {
        wxSizer* location_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
        location_body_hsizer->Add(folders_text_ctrl_, 1, wxALIGN_CENTER_VERTICAL);
        body_vsizer->Add(location_body_hsizer, 1, wxEXPAND);
      } else {
        body_vsizer->AddStretchSpacer(1);
      }

      body_vsizer->AddSpacer(2);
    }

    wxSizer* find_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
    find_body_hsizer->Add(find_combobox_, 1, wxALIGN_CENTER_VERTICAL);
    body_vsizer->Add(find_body_hsizer, 1, wxEXPAND);

    if (with_replace) {
      body_vsizer->AddSpacer(2);
      wxSizer* replace_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
      replace_body_hsizer->Add(replace_combobox_, 1, wxALIGN_CENTER_VERTICAL);
      body_vsizer->Add(replace_body_hsizer, 1, wxEXPAND);
    }
  }

  wxSizer* foot_vsizer = new wxBoxSizer(wxVERTICAL);
  {
    if (with_location) {
      foot_vsizer->AddStretchSpacer(1);
      foot_vsizer->AddSpacer(2);
    }

    wxSizer* find_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
    find_foot_hsizer->Add(find_button_, 0, wxALIGN_CENTER_VERTICAL);
    find_foot_hsizer->Add(find_all_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
    foot_vsizer->Add(find_foot_hsizer, 1);

    if (with_replace) {
      foot_vsizer->AddSpacer(2);
      wxSizer* replace_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
      replace_foot_hsizer->Add(replace_button_, 0, wxALIGN_CENTER_VERTICAL);
      replace_foot_hsizer->Add(replace_all_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
      foot_vsizer->Add(replace_foot_hsizer, 1);
    }
  }

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);
  ctrl_hsizer->Add(head_vsizer, 0, wxEXPAND);
  ctrl_hsizer->Add(body_vsizer, 1, wxEXPAND | wxLEFT, kPadding);
  ctrl_hsizer->Add(foot_vsizer, 0, wxEXPAND | wxLEFT, kPadding);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->AddSpacer(2);  // Top borders
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPadding);
  SetSizer(vsizer);

  Layout();
}

void FindPanel::AddToggleButtons(wxSizer* hsizer) {
  hsizer->Add(location_toggle_button_, 0, wxALIGN_CENTER_VERTICAL, 0);
  hsizer->Add(use_regex_toggle_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  hsizer->Add(case_sensitive_toggle_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
  hsizer->Add(match_word_toggle_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
}

void FindPanel::ShowReplace(bool show) {
  replace_combobox_->Show(show);
  replace_button_->Show(show);
  replace_all_button_->Show(show);
}

void FindPanel::ShowLocation(bool show) {
  location_combobox_->Show(show);
  folders_text_ctrl_->Show(show && location_ == kFolders);
}

// TODO
void FindPanel::InitComboStyle() {
  combo_style_.reset(new ui::ComboStyle);

  combo_style_->SetColor(ui::ComboStyle::BG, ui::ComboStyle::NORMAL, theme_->GetColor(BG_TOP));
  combo_style_->SetColor(ui::ComboStyle::FG, ui::ComboStyle::NORMAL, *wxWHITE);
  combo_style_->SetColor(ui::ComboStyle::BORDER, ui::ComboStyle::NORMAL, wxColour(125, 147, 156));
  combo_style_->SetColor(ui::ComboStyle::ARROW_BORDER, ui::ComboStyle::NORMAL, *wxWHITE);
  combo_style_->SetColor(ui::ComboStyle::ARROW_FILL, ui::ComboStyle::NORMAL, *wxWHITE);

  combo_style_->Fix();
}

void FindPanel::InitButtonStyle() {
  button_style_.reset(new ui::ButtonStyle);

  editor::SharedTheme button_theme = theme_->GetTheme(BUTTON);
  if (!button_theme) {
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

  button_style_->Fix();
}

ui::BitmapToggleButton* FindPanel::NewToggleButton(int id, const wxString& bitmap) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(button_style_);
  button->Create(this, id);
  button->SetBitmap(skin::GetIcon(bitmap));
  button->set_user_best_size(wxSize(24, 24));
  return button;
}

ui::TextButton* FindPanel::NewTextButton(int id, const wxString& label) {
  ui::TextButton* button = new ui::TextButton(button_style_);
  button->Create(this, id, label);
  button->SetMinSize(wxSize(80, -1));
  return button;
}

void FindPanel::PostEvent(int event_type,
                          const wxString& find_str,
                          const wxString& replace_str) {
  FindPanelEvent evt(GetId());
  evt.SetEventObject(this);
  evt.SetInt(event_type);
  evt.set_flags(flags_);
  evt.set_find_str(find_str.ToStdWstring());
  evt.set_replace_str(replace_str.ToStdWstring());
  evt.set_location(kCurrentPage);  // TODO
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace jil
