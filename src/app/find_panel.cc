#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/combobox.h"
#include "wx/dcbuffer.h"
#include "wx/dirdlg.h"
#include "wx/filename.h"
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

#include "app/book_frame.h"
#include "app/id.h"
#include "app/session.h"
#include "app/text_page.h"
#include "app/util.h"

#define kTrCurrentPage        _("Current Page")
#define kTrAllPages           _("All Pages")
#define kTrFolders            _("Folders")

#define kTrLocation           _("Choose location")
#define kTrUseRegex           _("Use regular expression")
#define kTrCaseSensitive      _("Case sensitive")
#define kTrMatchWord          _("Match whole word")

#define kTrFind               _("Find")
#define kTrReplace            _("Replace")
#define kTrFindAll            _("Find All")
#define kTrReplaceAll         _("Replace All")

namespace jil {

const int kPaddingX = 5;
const int kPaddingY = 2;

static const wxChar kFolderSp = wxT(';');

DEFINE_EVENT_TYPE(kFindPanelEvent)

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
EVT_PAINT(FindPanel::OnPaint)

EVT_MENU(ID_FP_MENU_CURRENT_PAGE, FindPanel::OnMenuCurrentPage)
EVT_MENU(ID_FP_MENU_ALL_PAGES, FindPanel::OnMenuAllPages)
EVT_MENU(ID_FP_MENU_FOLDERS, FindPanel::OnMenuFolders)

EVT_BUTTON(ID_FP_LOCATION_BUTTON, FindPanel::OnLocationButtonClick)
EVT_BUTTON(ID_FP_ADD_FOLDER_BUTTON, FindPanel::OnAddFolderButtonClick)

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
    : book_frame_(NULL)
    , session_(NULL)
    , mode_(kFindMode)
    , flags_(0)
    , location_(kCurrentPage) {
}

FindPanel::FindPanel(Session* session, int mode)
    : book_frame_(NULL)
    , session_(session)
    , mode_(mode)
    , flags_(0)
    , location_(kCurrentPage) {
}

bool FindPanel::Create(BookFrame* book_frame, wxWindowID id) {
  assert(theme_);
  assert(session_ != NULL);

  book_frame_ = book_frame;

  // Restore find options from session.
  flags_ = session_->find_flags();
  location_ = session_->find_location();

  if (!wxPanel::Create(book_frame, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(theme_->GetColor(COLOR_BG_TOP));

  InitButtonStyle();

  //----------------------------------------------------------------------------

  folders_label_ = new wxStaticText(this, ID_FP_FOLDERS_LABEL, kTrFolders);
  folders_label_->SetForegroundColour(theme_->GetColor(COLOR_FG));
  folders_label_->Hide();

  folders_text_ctrl_ = new wxTextCtrl(this, ID_FP_FOLDERS_TEXTCTRL);
  folders_text_ctrl_->Hide();

  add_folder_button_ = NewBitmapButton(ID_FP_ADD_FOLDER_BUTTON, IMAGE_ADD);
  add_folder_button_->Hide();

  //----------------------------------------------------------------------------

  location_button_ = NewBitmapButton(ID_FP_LOCATION_BUTTON, IMAGE_LOCATION);
  location_button_->SetToolTip(kTrLocation);

  use_regex_tbutton_ = NewBitmapToggleButton(ID_FP_USE_REGEX_TBUTTON, IMAGE_USE_REGEX);
  use_regex_tbutton_->SetToolTip(kTrUseRegex);

  case_sensitive_tbutton_ = NewBitmapToggleButton(ID_FP_CASE_SENSITIVE_TBUTTON, IMAGE_CASE_SENSITIVE);
  case_sensitive_tbutton_->SetToolTip(kTrCaseSensitive);

  match_word_tbutton_ = NewBitmapToggleButton(ID_FP_MATCH_WORD_TBUTTON, IMAGE_MATCH_WORD);
  match_word_tbutton_->SetToolTip(kTrMatchWord);

  // Initialize toggle button states.
  use_regex_tbutton_->set_toggle(GetBit(flags_, kFind_UseRegex));
  case_sensitive_tbutton_->set_toggle(GetBit(flags_, kFind_CaseSensitive));
  match_word_tbutton_->set_toggle(GetBit(flags_, kFind_MatchWord));

  //------------------------------------

  find_combobox_ = new wxComboBox(this, ID_FP_FIND_COMBOBOX);
  find_combobox_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
  InitFindComboBox();

  //------------------------------------

  replace_combobox_ = new wxComboBox(this, ID_FP_REPLACE_COMBOBOX);
  replace_combobox_->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
  InitReplaceComboBox();

  //------------------------------------

  find_button_ = NewTextButton(ID_FP_FIND_BUTTON, kTrFind);
  find_all_button_ = NewTextButton(ID_FP_FIND_ALL_BUTTON, kTrFindAll);
  replace_button_ = NewTextButton(ID_FP_REPLACE_BUTTON, kTrReplace);
  replace_all_button_ = NewTextButton(ID_FP_REPLACE_ALL_BUTTON, kTrReplaceAll);

  //------------------------------------

  SetSizer(new wxBoxSizer(wxVERTICAL));

  ShowFolders(location_ == kFolders);

  UpdateLayout();

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
  wxColour bg_top = theme_->GetColor(COLOR_BG_TOP);
  wxColour bg_bottom = theme_->GetColor(COLOR_BG_BOTTOM);
  dc.GradientFillLinear(bg_rect, bg_bottom, bg_top, wxNORTH);

  // Borders
  int border_y = rect.y;
  dc.SetPen(wxPen(theme_->GetColor(COLOR_BORDER_OUTER)));
  dc.DrawLine(bg_rect.x, border_y, bg_rect.GetRight() + 1, border_y);
  ++border_y;
  dc.SetPen(wxPen(theme_->GetColor(COLOR_BORDER_INNER)));
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

  // Add current working dir.
  if (folders_text_ctrl_->GetValue().IsEmpty()) {
    wxString cwd = wxGetCwd();
    folders_text_ctrl_->SetValue(cwd);
  }
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
    menu.Check(ID_FP_MENU_CURRENT_PAGE, true);
  } else if (location_ == kAllPages) {
    menu.Check(ID_FP_MENU_ALL_PAGES, true);
  } else {
    menu.Check(ID_FP_MENU_FOLDERS, true);
  }

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void FindPanel::OnAddFolderButtonClick(wxCommandEvent& evt) {
  wxString default_path = wxGetCwd();
  long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST;
  wxDirDialog dlg(this, wxDirSelectorPromptStr, default_path, style);

  if (dlg.ShowModal() == wxID_OK) {
    AddFolder(dlg.GetPath());
  }
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
  if (location_ == kCurrentPage) {
    HandleFind();
  }
}

void FindPanel::OnFindAll(wxCommandEvent& evt) {
  wxString find_str = find_combobox_->GetValue();
  if (find_str.IsEmpty()) {
    return;
  }

  AddFindString(find_str);

  std::wstring str = find_str.ToStdWstring();

  if (location_ == kCurrentPage) {
    book_frame_->FindAllInActivePage(str, flags_);

  } else if (location_ == kAllPages) {
    book_frame_->FindAllInAllPages(str, flags_);

  } else if (location_ == kFolders) {
    wxString folders_str = folders_text_ctrl_->GetValue();
    folders_str.Trim(true).Trim(false);
    if (!folders_str.IsEmpty()) {
      wxArrayString folders = wxSplit(folders_str, kFolderSp);
      book_frame_->FindAllInFolders(str, flags_, folders);
    }
  }
}

void FindPanel::OnReplace(wxCommandEvent& evt) {
  HandleReplace(false);
}

void FindPanel::OnReplaceAll(wxCommandEvent& evt) {
  HandleReplace(true);
}

void FindPanel::OnFindText(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    wxString find_str = find_combobox_->GetValue();
    // Post event even if the find string is empty so that the previous matching
    // results can be cleared.
    PostEvent(kFindStrEvent, find_str, wxEmptyString);
  }
}

void FindPanel::OnFindTextEnter(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    HandleFind();
  }
}

void FindPanel::HandleFind() {
  assert(location_ == kCurrentPage);

  wxString find_str = find_combobox_->GetValue();
  if (find_str.IsEmpty()) {
    return;
  }

  AddFindString(find_str);

  book_frame_->FindInActivePage(find_str.ToStdWstring(), flags_);
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

void FindPanel::SetLocation(FindLocation location) {
  if (location_ != location) {
    bool folders = location_ == kFolders || location == kFolders;
    location_ = location;

    if (folders) {
      ShowFolders(location == kFolders);
      PostEvent(kLayoutEvent, wxEmptyString, wxEmptyString);
    }

    UpdateLayout();
  }
}

void FindPanel::InitFindComboBox() {
  const std::list<wxString>& find_strings = session_->find_strings();
  for (const wxString& str : find_strings) {
    find_combobox_->Append(str);
  }

  if (!find_combobox_->IsListEmpty()) {
    find_combobox_->Select(0);
  }

  if (!find_strings.empty()) {
    find_combobox_->SetValue(find_strings.front());
  }
}

void FindPanel::InitReplaceComboBox() {
  const std::list<wxString>& replace_strings = session_->replace_strings();
  for (const wxString& str : replace_strings) {
    replace_combobox_->Append(str);
  }

  if (!replace_combobox_->IsListEmpty()) {
    replace_combobox_->Select(0);
  }
}

// TODO: If a folder includes another folder...
void FindPanel::AddFolder(const wxString& folder) {
  wxString folders_str = folders_text_ctrl_->GetValue();
  folders_str.Trim(true).Trim(false);

  if (!folders_str.IsEmpty()) {
    wxArrayString folders = wxSplit(folders_str, kFolderSp);

    // Check if the folder already exists.
    size_t count = folders.GetCount();
    wxFileName new_fn = wxFileName::DirName(folder);
    for (size_t i = 0; i < count; ++i) {
      wxFileName fn = wxFileName::DirName(folders[i]);
      if (fn.IsOk() && fn == new_fn) {
        return;
      }
    }
  }

  if (!folders_str.IsEmpty() && !folders_str.EndsWith(wxT(";"))) {
    folders_str += kFolderSp;
  }

  folders_str += folder;

  folders_text_ctrl_->SetValue(folders_str);
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
  CommonLayout(false);
  Thaw();
}

void FindPanel::LayoutAsReplace() {
  Freeze();
  find_button_->Show(location_ == kCurrentPage);
  ShowReplace(true);
  CommonLayout(true);
  Thaw();
}

void FindPanel::CommonLayout(bool with_replace) {
  wxSizer* head_vsizer = CommonLayoutHead(with_replace);
  wxSizer* body_vsizer = CommonLayoutBody(with_replace);
  wxSizer* foot_vsizer = CommonLayoutFoot(with_replace);

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);
  ctrl_hsizer->Add(head_vsizer, 0, wxEXPAND);
  ctrl_hsizer->Add(body_vsizer, 1, wxEXPAND | wxLEFT, kPaddingX);
  ctrl_hsizer->Add(foot_vsizer, 0, wxEXPAND | wxLEFT, kPaddingX);

  wxSizer* vsizer = GetSizer();
  vsizer->Clear(false);
  vsizer->AddSpacer(2);  // Top borders
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPaddingX);

  Layout();
}

wxSizer* FindPanel::CommonLayoutHead(bool with_replace) {
  wxSizer* head_vsizer = new wxBoxSizer(wxVERTICAL);

  if (location_ == kFolders) {
    wxSizer* location_head_hsizer = new wxBoxSizer(wxHORIZONTAL);
    location_head_hsizer->AddStretchSpacer(1);
    location_head_hsizer->Add(folders_label_, 0, wxALIGN_CV);
    head_vsizer->Add(location_head_hsizer, 1, wxEXPAND);
    head_vsizer->AddSpacer(kPaddingY);
  }

  wxSizer* find_head_hsizer = new wxBoxSizer(wxHORIZONTAL);
  AddOptionButtons(find_head_hsizer);
  head_vsizer->Add(find_head_hsizer, 1, wxEXPAND);

  if (with_replace) {
    head_vsizer->AddSpacer(kPaddingY);
    head_vsizer->AddStretchSpacer(1);
  }

  return head_vsizer;
}

wxSizer* FindPanel::CommonLayoutBody(bool with_replace) {
  wxSizer* body_vsizer = new wxBoxSizer(wxVERTICAL);

  if (location_ == kFolders) {
    wxSizer* location_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
    location_body_hsizer->Add(folders_text_ctrl_, 1, wxALIGN_CV);
    body_vsizer->Add(location_body_hsizer, 1, wxEXPAND);
    body_vsizer->AddSpacer(kPaddingY);
  }

  wxSizer* find_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_body_hsizer->Add(find_combobox_, 1, wxALIGN_CV);
  body_vsizer->Add(find_body_hsizer, 1, wxEXPAND);

  if (with_replace) {
    body_vsizer->AddSpacer(kPaddingY);
    wxSizer* replace_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
    replace_body_hsizer->Add(replace_combobox_, 1, wxALIGN_CV);
    body_vsizer->Add(replace_body_hsizer, 1, wxEXPAND);
  }

  return body_vsizer;
}

wxSizer* FindPanel::CommonLayoutFoot(bool with_replace) {
  wxSizer* foot_vsizer = new wxBoxSizer(wxVERTICAL);

  if (location_ == kFolders) {
    wxSizer* location_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
    location_foot_hsizer->Add(add_folder_button_, 0, wxALIGN_CV);
    foot_vsizer->Add(location_foot_hsizer, 1, wxEXPAND);
    foot_vsizer->AddSpacer(kPaddingY);
  }

  wxSizer* find_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
  if (find_button_->IsShown()) {
    find_foot_hsizer->Add(find_button_, 0, wxALIGN_CV);
    find_foot_hsizer->Add(find_all_button_, 0, wxALIGN_CV | wxLEFT, kPaddingX);
  } else {
    find_foot_hsizer->Add(find_all_button_, 0, wxALIGN_CV);
  }
  foot_vsizer->Add(find_foot_hsizer, 1);

  if (with_replace) {
    foot_vsizer->AddSpacer(kPaddingY);
    wxSizer* replace_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
    if (replace_button_->IsShown()) {
      replace_foot_hsizer->Add(replace_button_, 0, wxALIGN_CV);
      replace_foot_hsizer->Add(replace_all_button_, 0, wxALIGN_CV | wxLEFT, kPaddingX);
    } else {
      replace_foot_hsizer->Add(replace_all_button_, 0, wxALIGN_CV);
    }
    foot_vsizer->Add(replace_foot_hsizer, 1);
  }

  return foot_vsizer;
}

void FindPanel::AddOptionButtons(wxSizer* hsizer) {
  hsizer->Add(location_button_, 0, wxALIGN_CV, 0);
  hsizer->Add(use_regex_tbutton_, 0, wxALIGN_CV | wxLEFT, 5);
  hsizer->Add(case_sensitive_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);
  hsizer->Add(match_word_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);
}

void FindPanel::ShowReplace(bool show) {
  replace_combobox_->Show(show);
  replace_button_->Show(show && location_ == kCurrentPage);
  replace_all_button_->Show(show);
}

void FindPanel::ShowFolders(bool show) {
  folders_label_->Show(show);
  folders_text_ctrl_->Show(show);
  add_folder_button_->Show(show);
}

void FindPanel::InitButtonStyle() {
  button_style_.reset(new ui::ButtonStyle);

  editor::SharedTheme button_theme = theme_->GetTheme(THEME_BUTTON);
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

ui::BitmapButton* FindPanel::NewBitmapButton(int id, ImageId image_id) {
  ui::BitmapButton* button = new ui::BitmapButton(button_style_);
  button->Create(this, id);
  button->SetBitmap(theme_->GetImage(image_id));
  button->set_user_best_size(wxSize(24, 24));  // TODO
  return button;
}

ui::BitmapToggleButton* FindPanel::NewBitmapToggleButton(int id, ImageId image_id) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(button_style_);
  button->Create(this, id);
  button->SetBitmap(theme_->GetImage(image_id));
  button->set_user_best_size(wxSize(24, 24));  // TODO
  return button;
}

ui::TextButton* FindPanel::NewTextButton(int id, const wxString& label) {
  ui::TextButton* button = new ui::TextButton(button_style_);
  button->Create(this, id, label);
  button->SetMinSize(wxSize(80, -1));  // TODO
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
