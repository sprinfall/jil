#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/combobox.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"

#include "ui/bitmap_button.h"
#include "ui/bitmap_toggle_button.h"
#include "ui/color.h"
#include "ui/label.h"
#include "ui/text_button.h"

#include "editor/text_buffer.h"

#include "app/id.h"
#include "app/session.h"
#include "app/skin.h"
#include "app/text_page.h"
#include "app/util.h"

#define kTrCurrentPage        _("Current Page")
#define kTrAllPages           _("All Pages")
#define kTrFolders            _("Folders")
#define kTrAddLineRange       _("Add Line Range")
#define kTrRemoveLineRange    _("Remove Line Range")
#define kTrAddFilters         _("Add Filters")
#define kTrRemoveFilters      _("Remove Filters")

#define kTrLocation           _("Show location panel")
#define kTrUseRegex           _("Use regular expression")
#define kTrCaseSensitive      _("Case sensitive")
#define kTrMatchWord          _("Match whole word")

#define kTrFind               _("Find")
#define kTrReplace            _("Replace")
#define kTrFindAll            _("Find All")
#define kTrReplaceAll         _("Replace All")

namespace jil {

const int kPadding = 5;

DEFINE_EVENT_TYPE(kFindPanelEvent)

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
EVT_PAINT(FindPanel::OnPaint)

EVT_MENU(ID_FP_MENU_CURRENT_PAGE, FindPanel::OnMenuCurrentPage)
EVT_MENU(ID_FP_MENU_ALL_PAGES, FindPanel::OnMenuAllPages)
EVT_MENU(ID_FP_MENU_FOLDERS, FindPanel::OnMenuFolders)
EVT_MENU(ID_FP_MENU_ADD_LINE_RANGE, FindPanel::OnMenuAddLineRange)
EVT_MENU(ID_FP_MENU_ADD_FILTERS, FindPanel::OnMenuAddFilters)

EVT_BUTTON(ID_FP_LOCATION_BUTTON, FindPanel::OnLocationButtonClick)
EVT_TOGGLEBUTTON(ID_FP_USE_REGEX_TBUTTON, FindPanel::OnUseRegexToggle)
EVT_TOGGLEBUTTON(ID_FP_CASE_SENSITIVE_TBUTTON, FindPanel::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON(ID_FP_MATCH_WORD_TBUTTON, FindPanel::OnMatchWordToggle)

EVT_BUTTON(ID_FP_FIND_BUTTON, FindPanel::OnFind)
EVT_BUTTON(ID_FP_FIND_ALL_BUTTON, FindPanel::OnFindAll)
EVT_BUTTON(ID_FP_REPLACE_BUTTON, FindPanel::OnReplace)
EVT_BUTTON(ID_FP_REPLACE_ALL_BUTTON, FindPanel::OnReplaceAll)

EVT_TEXT(ID_FP_FIND_COMBOBOX, FindPanel::OnFindText)
EVT_TEXT_ENTER(ID_FP_FIND_COMBOBOX, FindPanel::OnFindTextEnter)
END_EVENT_TABLE()

FindPanel::FindPanel()
    : session_(NULL)
    , mode_(kFindMode)
    , flags_(0)
    , location_(kCurrentPage) {
}

FindPanel::FindPanel(Session* session, int mode)
    : session_(session)
    , mode_(mode)
    , flags_(0)
    , location_(kCurrentPage) {
}

bool FindPanel::Create(wxWindow* parent, wxWindowID id) {
  assert(theme_);
  assert(session_ != NULL);

  // Restore find options from session.
  flags_ = session_->find_flags();

  //location_ = session_->find_location();

  show_locations_[kCurrentPage] = false;
  show_locations_[kAllPages] = false;
  show_locations_[kFolders] = true;

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(theme_->GetColor(BG_TOP));

  InitButtonStyle();

  //----------------------------------------------------------------------------

  location_label_ = new wxStaticText(this, ID_FP_LOCATION_LABEL, wxEmptyString);
  location_label_->SetForegroundColour(theme_->GetColor(FG));
  location_label_->SetLabel(GetLocationLabel(location_));
  location_label_->Hide();

  line_range_text_ctrl_ = new wxTextCtrl(this, ID_FP_LINE_RANGE_TEXTCTRL);
  line_range_text_ctrl_->Hide();

  filters_text_ctrl_ = new wxTextCtrl(this, ID_FP_FILTERS_TEXTCTRL);
  filters_text_ctrl_->Hide();

  folders_text_ctrl_ = new wxTextCtrl(this, ID_FP_FOLDERS_TEXTCTRL);
  folders_text_ctrl_->Hide();

  //----------------------------------------------------------------------------

  location_button_ = NewBitmapButton(ID_FP_LOCATION_BUTTON, wxT("location"));
  location_button_->SetToolTip(kTrLocation);

  use_regex_tbutton_ = NewBitmapToggleButton(ID_FP_USE_REGEX_TBUTTON, wxT("use_regex"));
  use_regex_tbutton_->SetToolTip(kTrUseRegex);

  case_sensitive_tbutton_ = NewBitmapToggleButton(ID_FP_CASE_SENSITIVE_TBUTTON, wxT("case_sensitive"));
  case_sensitive_tbutton_->SetToolTip(kTrCaseSensitive);

  match_word_tbutton_ = NewBitmapToggleButton(ID_FP_MATCH_WORD_TBUTTON, wxT("match_word"));
  match_word_tbutton_->SetToolTip(kTrMatchWord);

  // Initialize toggle button states.
  use_regex_tbutton_->set_toggle(GetBit(flags_, kFind_UseRegex));
  case_sensitive_tbutton_->set_toggle(GetBit(flags_, kFind_CaseSensitive));
  match_word_tbutton_->set_toggle(GetBit(flags_, kFind_MatchWord));

  //------------------------------------

  find_combobox_ = new wxComboBox(this, ID_FP_FIND_COMBOBOX);
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

  replace_combobox_ = new wxComboBox(this, ID_FP_REPLACE_COMBOBOX);

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

  find_button_ = NewTextButton(ID_FP_FIND_BUTTON, kTrFind);
  find_all_button_ = NewTextButton(ID_FP_FIND_ALL_BUTTON, kTrFindAll);
  replace_button_ = NewTextButton(ID_FP_REPLACE_BUTTON, kTrReplace);
  replace_all_button_ = NewTextButton(ID_FP_REPLACE_ALL_BUTTON, kTrReplaceAll);

  //find_button_->SetDefault();  // Set default for ENTER key.

  //------------------------------------

  ShowLocationCtrls(false);

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
  if (location_ != location) {
    location_ = location;

    location_label_->SetLabel(GetLocationLabel(location));
    ShowLocationCtrls(true);
  }
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

void FindPanel::OnMenuCurrentPage(wxCommandEvent& evt) {
  SetLocation(kCurrentPage);
}

void FindPanel::OnMenuAllPages(wxCommandEvent& evt) {
  SetLocation(kAllPages);
}

void FindPanel::OnMenuFolders(wxCommandEvent& evt) {
  SetLocation(kFolders);
}

void FindPanel::OnMenuAddLineRange(wxCommandEvent& evt) {
  show_locations_[kCurrentPage] = !show_locations_[kCurrentPage];
  ShowLocationCtrls(true);
}

void FindPanel::OnMenuAddFilters(wxCommandEvent& evt) {
  show_locations_[kAllPages] = !show_locations_[kAllPages];
  ShowLocationCtrls(true);
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_combobox_->SetValue(find_string);
  find_combobox_->SelectAll();
  AddFindString(find_string);
}

void FindPanel::OnLocationButtonClick(wxCommandEvent& evt) {
  wxMenu menu;

  menu.AppendCheckItem(ID_FP_MENU_CURRENT_PAGE, kTrCurrentPage);
  menu.AppendCheckItem(ID_FP_MENU_ALL_PAGES, kTrAllPages);
  menu.AppendCheckItem(ID_FP_MENU_FOLDERS, kTrFolders);
  
  if (location_ == kCurrentPage) {
    menu.AppendSeparator();

    if (show_locations_[kCurrentPage]) {
      menu.Append(ID_FP_MENU_ADD_LINE_RANGE, kTrRemoveLineRange);
    } else {
      menu.Append(ID_FP_MENU_ADD_LINE_RANGE, kTrAddLineRange);
    }

    menu.Check(ID_FP_MENU_CURRENT_PAGE, true);

  } else if (location_ == kAllPages) {
    menu.AppendSeparator();

    if (show_locations_[kAllPages]) {
      menu.Append(ID_FP_MENU_ADD_FILTERS, kTrRemoveFilters);
    } else {
      menu.Append(ID_FP_MENU_ADD_FILTERS, kTrAddFilters);
    }

    menu.Check(ID_FP_MENU_ALL_PAGES, true);

  } else {
    menu.Check(ID_FP_MENU_FOLDERS, true);
  }

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
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

  find_button_->Show(location_ == kCurrentPage);

  ShowReplace(false);

  if (show_locations_[location_]) {
    CommonLayout(true, false);
  } else {
    wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);

    AddOptionButtons(ctrl_hsizer);

    ctrl_hsizer->Add(find_combobox_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
    if (find_button_->IsShown()) {
      ctrl_hsizer->Add(find_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
    }
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

  find_button_->Show(location_ == kCurrentPage);

  ShowReplace(true);

  if (show_locations_[location_]) {
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
      location_head_hsizer->Add(location_label_, 1, wxALIGN_CENTER_VERTICAL);

      head_vsizer->Add(location_head_hsizer, 1, wxEXPAND);
      head_vsizer->AddSpacer(2);
    }
    
    wxSizer* find_head_hsizer = new wxBoxSizer(wxHORIZONTAL);
    AddOptionButtons(find_head_hsizer);
    head_vsizer->Add(find_head_hsizer, 1, wxEXPAND);

    if (with_replace) {
      head_vsizer->AddSpacer(2);
      head_vsizer->AddStretchSpacer(1);
    }
  }

  wxSizer* body_vsizer = new wxBoxSizer(wxVERTICAL);
  {
    if (with_location) {
      wxTextCtrl* text_ctrl = NULL;

      if (location_ == kCurrentPage) {
        text_ctrl = line_range_text_ctrl_;
      } else if (location_ == kAllPages) {
        text_ctrl = filters_text_ctrl_;
      } else {
        text_ctrl = folders_text_ctrl_;
      }

      wxSizer* location_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
      location_body_hsizer->Add(text_ctrl, 1, wxALIGN_CENTER_VERTICAL);
      body_vsizer->Add(location_body_hsizer, 1, wxEXPAND);

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
    if (find_button_->IsShown()) {
      find_foot_hsizer->Add(find_button_, 0, wxALIGN_CENTER_VERTICAL);
    }
    find_foot_hsizer->Add(find_all_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, kPadding);
    foot_vsizer->Add(find_foot_hsizer, 1);

    if (with_replace) {
      foot_vsizer->AddSpacer(2);
      wxSizer* replace_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
      if (replace_button_->IsShown()) {
        replace_foot_hsizer->Add(replace_button_, 0, wxALIGN_CENTER_VERTICAL);
      }
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

void FindPanel::AddOptionButtons(wxSizer* hsizer) {
  hsizer->Add(location_button_, 0, wxALIGN_CENTER_VERTICAL, 0);
  hsizer->Add(use_regex_tbutton_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
  hsizer->Add(case_sensitive_tbutton_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
  hsizer->Add(match_word_tbutton_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
}

void FindPanel::ShowReplace(bool show) {
  replace_combobox_->Show(show);
  replace_button_->Show(show && location_ == kCurrentPage);
  replace_all_button_->Show(show);
}

void FindPanel::ShowLocationCtrls(bool layout) {
  for (int i = 0; i < kFindLocations; ++i) {
    if (i != location_) {
      ShowLocationCtrls(static_cast<FindLocation>(i), false);
    }
  }

  bool show = show_locations_[location_];

  location_label_->Show(show);
  ShowLocationCtrls(location_, show);

  if (layout) {
    PostEvent(kLayoutEvent, wxEmptyString, wxEmptyString);
    UpdateLayout();
  }
}

void FindPanel::ShowLocationCtrls(FindLocation location, bool show) {
  if (location == kCurrentPage) {
    line_range_text_ctrl_->Show(show);
  } else if (location == kAllPages) {
    filters_text_ctrl_->Show(show);
  } else if (location == kFolders) {
    folders_text_ctrl_->Show(show);
  }
}

wxString FindPanel::GetLocationLabel(FindLocation location) const {
  if (location == kCurrentPage) {
    return kTrCurrentPage;
  } else if (location == kAllPages) {
    return kTrAllPages;
  } else {
    return kTrFolders;
  }
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

ui::BitmapButton* FindPanel::NewBitmapButton(int id, const wxString& bitmap) {
  ui::BitmapButton* button = new ui::BitmapButton(button_style_);
  button->Create(this, id);
  button->SetBitmap(skin::GetIcon(bitmap));
  button->set_user_best_size(wxSize(24, 24));
  return button;
}

ui::BitmapToggleButton* FindPanel::NewBitmapToggleButton(int id, const wxString& bitmap) {
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
