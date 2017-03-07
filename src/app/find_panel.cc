#include "app/find_panel.h"

#include "wx/button.h"
#include "wx/combobox.h"
#include "wx/dcbuffer.h"
#include "wx/dirdlg.h"
#include "wx/filename.h"
#include "wx/log.h"
#include "wx/menu.h"
#include "wx/sizer.h"
#include "wx/statbmp.h"
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
#define kTrFolder             _("Folder")

#define kTrUseRegex           _("Use regular expression")
#define kTrCaseSensitive      _("Case sensitive")
#define kTrMatchWord          _("Match whole word")
#define kTrReversely          _("Find reversely")

#define kTrFind               _("Find")
#define kTrReplace            _("Replace")
#define kTrFindAll            _("Find All")
#define kTrReplaceAll         _("Replace All")

namespace jil {

const int kPaddingX = 7;
const int kPaddingY = 2;

static const wxChar kFolderSp = wxT(';');

DEFINE_EVENT_TYPE(kFindPanelLayoutEvent)

////////////////////////////////////////////////////////////////////////////////

// Customized text ctrl for find and replace input.
// The command event kEvtTextHistory is posted when up or down key is pressed.

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(kEvtTextHistory, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(kEvtTextHistory)

#define EVT_TEXT_HISTORY(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(kEvtTextHistory, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

class FindTextCtrl : public wxTextCtrl {
  DECLARE_DYNAMIC_CLASS(FindTextCtrl)
  DECLARE_NO_COPY_CLASS(FindTextCtrl)
  DECLARE_EVENT_TABLE()

public:
  FindTextCtrl() {
  }

  FindTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString)
      : wxTextCtrl(parent, id, value) {
    SetWindowStyleFlag(wxTE_PROCESS_ENTER);
    SetLayoutDirection(wxLayout_LeftToRight);
  }

  void UseBoldFont(bool bold) {
    wxFont font = GetFont();
    font.SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    SetFont(font);
  }

protected:
  void OnKeyUp(wxKeyEvent& evt) {
    int code = evt.GetKeyCode();
    if (code == WXK_UP || code == WXK_DOWN) {
      wxCommandEvent cmd_evt(kEvtTextHistory, GetId());
      cmd_evt.SetEventObject(this);
      GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
      evt.Skip();
    }
  }
};

IMPLEMENT_DYNAMIC_CLASS(FindTextCtrl, wxTextCtrl)

BEGIN_EVENT_TABLE(FindTextCtrl, wxTextCtrl)
EVT_KEY_UP(FindTextCtrl::OnKeyUp)
END_EVENT_TABLE();

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
EVT_PAINT         (FindPanel::OnPaint)

EVT_MENU          (ID_FP_MENU_CURRENT_PAGE,       FindPanel::OnMenuCurrentPage)
EVT_MENU          (ID_FP_MENU_ALL_PAGES,          FindPanel::OnMenuAllPages)
EVT_MENU          (ID_FP_MENU_FOLDERS,            FindPanel::OnMenuFolders)

EVT_BUTTON        (ID_FP_ADD_FOLDER_BUTTON,       FindPanel::OnAddFolderButtonClick)

EVT_BUTTON        (ID_FP_LOCATION_BUTTON,         FindPanel::OnLocationButtonClick)

EVT_TOGGLEBUTTON  (ID_FP_USE_REGEX_TBUTTON,       FindPanel::OnUseRegexToggle)
EVT_TOGGLEBUTTON  (ID_FP_CASE_SENSITIVE_TBUTTON,  FindPanel::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON  (ID_FP_MATCH_WORD_TBUTTON,      FindPanel::OnMatchWordToggle)
EVT_TOGGLEBUTTON  (ID_FP_REVERSELY_TBUTTON,       FindPanel::OnReverselyToggle)

EVT_UPDATE_UI     (ID_FP_MATCH_WORD_TBUTTON,      FindPanel::OnMatchWordButtonUpdateUI)

EVT_BUTTON        (ID_FP_FIND_BUTTON,             FindPanel::OnFind)
EVT_BUTTON        (ID_FP_FIND_ALL_BUTTON,         FindPanel::OnFindAll)
EVT_BUTTON        (ID_FP_REPLACE_BUTTON,          FindPanel::OnReplace)
EVT_BUTTON        (ID_FP_REPLACE_ALL_BUTTON,      FindPanel::OnReplaceAll)

EVT_TEXT          (ID_FP_FIND_TEXTCTRL,           FindPanel::OnFindText)
EVT_TEXT_ENTER    (ID_FP_FIND_TEXTCTRL,           FindPanel::OnFindTextEnter)

EVT_TEXT_HISTORY  (ID_FP_FIND_TEXTCTRL,           FindPanel::OnFindTextHistory)
EVT_TEXT_HISTORY  (ID_FP_REPLACE_TEXTCTRL,        FindPanel::OnReplaceTextHistory)

EVT_MENU_RANGE    (ID_MENU_FIND_HISTORY_BEGIN,\
                   ID_MENU_FIND_HISTORY_END,\
                   FindPanel::OnFindTextHistoryMenu)

EVT_MENU_RANGE    (ID_MENU_REPLACE_HISTORY_BEGIN, \
                   ID_MENU_REPLACE_HISTORY_END, \
                   FindPanel::OnReplaceTextHistoryMenu)
END_EVENT_TABLE()

FindPanel::FindPanel()
    : book_frame_(NULL)
    , session_(NULL)
    , mode_(kFindMode)
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

  button_style_ = ButtonStyleFromTheme(theme_->GetTheme(THEME_BUTTON));

  //----------------------------------------------------------------------------

  folder_label_ = new ui::Label(this, ID_FP_FOLDER_LABEL, kTrFolder);
  folder_label_->Hide();

  folder_text_ctrl_ = new wxTextCtrl(this, ID_FP_FOLDER_TEXTCTRL);
  folder_text_ctrl_->Hide();

  int height = folder_text_ctrl_->GetBestSize().GetHeight();
  bitmap_button_best_size_.Set(height, height);

  add_folder_button_ = NewBitmapButton(ID_FP_ADD_FOLDER_BUTTON);
  add_folder_button_->Hide();

  //----------------------------------------------------------------------------

  location_button_ = NewBitmapButton(ID_FP_LOCATION_BUTTON, false, false);

  use_regex_tbutton_ = NewBitmapToggleButton(ID_FP_USE_REGEX_TBUTTON);
  use_regex_tbutton_->SetToolTip(kTrUseRegex);

  case_sensitive_tbutton_ = NewBitmapToggleButton(ID_FP_CASE_SENSITIVE_TBUTTON);
  case_sensitive_tbutton_->SetToolTip(kTrCaseSensitive);

  match_word_tbutton_ = NewBitmapToggleButton(ID_FP_MATCH_WORD_TBUTTON);
  match_word_tbutton_->SetToolTip(kTrMatchWord);

  reversely_tbutton_ = NewBitmapToggleButton(ID_FP_REVERSELY_TBUTTON);
  reversely_tbutton_->SetToolTip(kTrReversely);

  // Initialize toggle button states.
  use_regex_tbutton_->set_toggle(GetBit(flags_, kFind_UseRegex));
  case_sensitive_tbutton_->set_toggle(GetBit(flags_, kFind_CaseSensitive));
  match_word_tbutton_->set_toggle(GetBit(flags_, kFind_MatchWord));
  reversely_tbutton_->set_toggle(GetBit(flags_, kFind_Reversely));

  //------------------------------------

  find_text_ctrl_ = new FindTextCtrl(this, ID_FP_FIND_TEXTCTRL);
  replace_text_ctrl_ = new FindTextCtrl(this, ID_FP_REPLACE_TEXTCTRL);

  // Save normal foreground color.
  fg_color_ = find_text_ctrl_->GetForegroundColour();
  invalid_fg_color_ = *wxRED;

  //------------------------------------

  find_button_ = NewTextButton(ID_FP_FIND_BUTTON, kTrFind);
  find_all_button_ = NewTextButton(ID_FP_FIND_ALL_BUTTON, kTrFindAll);
  replace_button_ = NewTextButton(ID_FP_REPLACE_BUTTON, kTrReplace);
  replace_all_button_ = NewTextButton(ID_FP_REPLACE_ALL_BUTTON, kTrReplaceAll);

  //------------------------------------

  // Apply colors and bitmaps.
  ReapplyTheme();

  //------------------------------------

  SetSizer(new wxBoxSizer(wxVERTICAL));

  ShowFolders(location_ == kFolders);

  UpdateLayout();

  // Restore last find string.
  if (!session_->find_strings().empty()) {
    find_text_ctrl_->SetValue(session_->find_strings().front());
  }

  if (mode_ == kReplaceMode) {
    if (!session_->replace_strings().empty()) {
      replace_text_ctrl_->SetValue(session_->replace_strings().front());
    }
  }

  return true;
}

FindPanel::~FindPanel() {
}

bool FindPanel::Destroy() {
  session_->set_find_flags(flags_);
  session_->set_find_location(location_);
  return wxPanel::Destroy();
}

void FindPanel::SetFocus() {
  wxPanel::SetFocus();

  find_text_ctrl_->SetFocus();

  if (!find_text_ctrl_->GetValue().IsEmpty()) {
    find_text_ctrl_->SelectAll();
  }
}

bool FindPanel::SetFont(const wxFont& font) {
  bool result = wxPanel::SetFont(font);

  wxWindowList& children = GetChildren();
  for (size_t i = 0; i < children.GetCount(); ++i) {
    children[i]->SetFont(font);
    children[i]->Refresh();
  }

  if (GetBit(flags_, kFind_UseRegex)) {
    find_text_ctrl_->UseBoldFont(true);
  }

  Layout();

  return result;
}

void FindPanel::UpdateLayout() {
  if (mode_ == kFindMode) {
    LayoutAsFind();
  } else {
    LayoutAsReplace();
  }
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_text_ctrl_->SetValue(find_string);
  find_text_ctrl_->SelectAll();
  AddFindString(find_string);
}

void FindPanel::ReapplyTheme() {
  assert(theme_);

  button_style_ = ButtonStyleFromTheme(theme_->GetTheme(THEME_BUTTON));

  folder_label_->SetForegroundColour(theme_->GetColor(COLOR_FG));

  add_folder_button_->SetBitmaps(theme_->GetImage(IMAGE_ADD_FOLDER));

  UpdateLocationButton();

  SetButtonBitmapsNH(use_regex_tbutton_,
                     IMAGE_USE_REGEX,
                     IMAGE_USE_REGEX_HOVER);

  SetButtonBitmapsNH(case_sensitive_tbutton_,
                     IMAGE_CASE_SENSITIVE,
                     IMAGE_CASE_SENSITIVE_HOVER);

  SetButtonBitmapsNHD(match_word_tbutton_,
                      IMAGE_MATCH_WORD,
                      IMAGE_MATCH_WORD_HOVER,
                      IMAGE_MATCH_WORD_DISABLED);

  SetButtonBitmapsNHD(reversely_tbutton_,
                      IMAGE_REVERSELY,
                      IMAGE_REVERSELY_HOVER,
                      IMAGE_REVERSELY_DISABLED);
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
}

void FindPanel::OnAddFolderButtonClick(wxCommandEvent& evt) {
  wxString default_path = wxGetCwd();
  long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST;
  wxDirDialog dlg(this, wxDirSelectorPromptStr, default_path, style);

  if (dlg.ShowModal() == wxID_OK) {
    AddFolder(dlg.GetPath());
  }
}

void FindPanel::OnLocationButtonClick(wxCommandEvent& evt) {
  SetLocation(GetNextLocation(location_));
  UpdateLocationButton();
}

void FindPanel::OnUseRegexToggle(wxCommandEvent& evt) {
  bool use_regex = evt.IsChecked();
  flags_ = SetBit(flags_, kFind_UseRegex, use_regex);

  find_text_ctrl_->UseBoldFont(use_regex);

  // A valid find text might become invalid after Use Regex option is
  // switched on.
  HandleFindTextChange();
}

void FindPanel::OnCaseSensitiveToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_CaseSensitive, evt.IsChecked());

  if (location_ == kCurrentPage) {
    FindIncrementally(find_text_ctrl_->GetValue());
  }
}

void FindPanel::OnMatchWordToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_MatchWord, evt.IsChecked());

  if (location_ == kCurrentPage) {
    FindIncrementally(find_text_ctrl_->GetValue());
  }
}

void FindPanel::OnReverselyToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_Reversely, evt.IsChecked());
}

void FindPanel::OnMatchWordButtonUpdateUI(wxUpdateUIEvent& evt) {
  // NOTE (Disable Match Whole Word option if use regex):
  // You can't just surround the regex string with '\b' to match the whole
  // word, because '\b' can't match EOL.
  // So disable "Match Whole Word" when "Use Regular Expression" is on.
  match_word_tbutton_->Enable(!GetBit(flags_, kFind_UseRegex));
}

void FindPanel::OnFind(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    HandleFind();
  }  // else: Not supported.
}

void FindPanel::OnFindAll(wxCommandEvent& evt) {
  wxString find_wxstr = find_text_ctrl_->GetValue();
  if (find_wxstr.IsEmpty()) {
    book_frame_->SetLastFindStringAndFlags(L"", flags_);
    return;
  }

  AddFindString(find_wxstr);

  std::wstring find_str = find_wxstr.ToStdWstring();

  book_frame_->SetLastFindStringAndFlags(find_str, flags_);

  if (location_ == kCurrentPage) {
    book_frame_->FindAllInActivePage(find_str, flags_);
  } else if (location_ == kAllPages) {
    book_frame_->FindAllInAllPages(find_str, flags_);
  } else if (location_ == kFolders) {
    wxString folder = GetFolder();
    if (!folder.IsEmpty()) {
      book_frame_->FindAllInFolder(find_str, flags_, folder);
    }
  }
}

void FindPanel::OnReplace(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    HandleReplace(false);
  }  // else: Not supported.
}

void FindPanel::OnReplaceAll(wxCommandEvent& evt) {
  HandleReplace(true);
}

void FindPanel::OnFindText(wxCommandEvent& evt) {
  HandleFindTextChange();
}

void FindPanel::OnFindTextEnter(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    HandleFind();
  }
}

void FindPanel::HandleFindTextChange() {
  wxString find_wxstr = find_text_ctrl_->GetValue();
  bool valid = IsFindStringValid(find_wxstr, false);

  EnableButtons(valid);
  SetFindTextCtrlFgColor(find_wxstr.IsEmpty() || valid);

  // NOTE:
  // Find incrementally even if the find string is empty or invalid so that
  // the previous find results can be cleared.
  if (location_ == kCurrentPage) {
    FindIncrementally(find_wxstr);
  }
}

void FindPanel::FindIncrementally(const wxString& find_wxstr) {
  assert(location_ == kCurrentPage);

  std::wstring find_str = find_wxstr.ToStdWstring();
  book_frame_->SetLastFindStringAndFlags(find_str, flags_);
  book_frame_->FindInActivePageIncrementally(find_str, flags_);
}

void FindPanel::OnFindTextHistory(wxCommandEvent& evt) {
  ShowHistoryMenu(session_->find_strings(),
                  ID_MENU_FIND_HISTORY_BEGIN,
                  find_text_ctrl_);
}

void FindPanel::OnReplaceTextHistory(wxCommandEvent& evt) {
  ShowHistoryMenu(session_->replace_strings(),
                  ID_MENU_REPLACE_HISTORY_BEGIN,
                  replace_text_ctrl_);
}

void FindPanel::ShowHistoryMenu(const std::list<wxString>& history_strings,
                                int id_begin,
                                wxTextCtrl* text_ctrl) {
  if (history_strings.empty()) {
    return;
  }

  wxMenu menu;

  int menu_id = id_begin;

  for (const wxString& str : history_strings) {
    menu.Append(menu_id, str);
    ++menu_id;
  }

  wxPoint pos = text_ctrl->GetScreenPosition();
  PopupMenu(&menu, ScreenToClient(pos));
}

void FindPanel::OnFindTextHistoryMenu(wxCommandEvent& evt) {
  const std::list<wxString>& find_strings = session_->find_strings();
  HandleHistoryMenu(find_strings, evt.GetId(), find_text_ctrl_);
}

void FindPanel::OnReplaceTextHistoryMenu(wxCommandEvent& evt) {
  const std::list<wxString>& replace_strings = session_->replace_strings();
  HandleHistoryMenu(replace_strings, evt.GetId(), replace_text_ctrl_);
}

void FindPanel::HandleHistoryMenu(const std::list<wxString>& history_strings,
                                  int menu_id,
                                  wxTextCtrl* text_ctrl) {
  size_t index = static_cast<size_t>(menu_id - ID_MENU_FIND_HISTORY_BEGIN);
  if (index >= history_strings.size()) {
    return;
  }

  std::list<wxString>::const_iterator it = history_strings.begin();
  std::advance(it, index);
  text_ctrl->SetValue(*it);
  text_ctrl->SetInsertionPointEnd();
}

void FindPanel::HandleFind() {
  assert(location_ == kCurrentPage);

  wxString find_wxstr = find_text_ctrl_->GetValue();
  if (find_wxstr.IsEmpty()) {
    book_frame_->SetLastFindStringAndFlags(L"", flags_);
    return;
  }

  AddFindString(find_wxstr);

  std::wstring find_str = find_wxstr.ToStdWstring();
  book_frame_->SetLastFindStringAndFlags(find_str, flags_);
  book_frame_->FindInActivePage(find_str, flags_);
}

void FindPanel::HandleReplace(bool all) {
  wxString find_wxstr = find_text_ctrl_->GetValue();
  if (find_wxstr.IsEmpty()) {
    // Clear last find string.
    book_frame_->SetLastFindStringAndFlags(L"", flags_);
    return;
  }

  AddFindString(find_wxstr);

  wxString replace_wxstr = replace_text_ctrl_->GetValue();
  if (!replace_wxstr.IsEmpty()) {
    AddReplaceString(replace_wxstr);
  }

  std::wstring find_str = find_wxstr.ToStdWstring();
  std::wstring replace_str = replace_wxstr.ToStdWstring();

  book_frame_->SetLastFindStringAndFlags(find_str, flags_);

  if (location_ == kCurrentPage) {
    if (all) {
      book_frame_->ReplaceAllInActivePage(find_str, replace_str, flags_);
    } else {
      book_frame_->ReplaceInActivePage(find_str, replace_str, flags_);
    }
  } else if (location_ == kAllPages) {
    if (all) {
      book_frame_->ReplaceAllInAllPages(find_str, replace_str, flags_);
    }  // else: Not supported
  } else if (location_ == kFolders) {
    if (all) {
      wxString folder = GetFolder();
      if (!folder.IsEmpty()) {
        book_frame_->ReplaceAllInFolder(find_str, replace_str, flags_, folder);
      }
    }  // else: Not supported
  }
}

void FindPanel::SetLocation(FindLocation location) {
  if (location_ == location) {
    return;
  }

  bool folders = location_ == kFolders || location == kFolders;
  location_ = location;

  if (folders) {
    ShowFolders(location == kFolders);
    PostLayoutEvent();
  }

  UpdateLayout();

  if (location_ == kFolders) {
    // Add current working dir.
    if (folder_text_ctrl_->GetValue().IsEmpty()) {
      wxString cwd = wxGetCwd();
      folder_text_ctrl_->SetValue(cwd);
    }
  }

  // Support Reversely option only for finding in current page.
  reversely_tbutton_->Enable(location_ == kCurrentPage);
}

FindLocation FindPanel::GetNextLocation(FindLocation location) const {
  if (location_ == kCurrentPage) {
    return kAllPages;
  }
  if (location_ == kAllPages) {
    return kFolders;
  }
  return kCurrentPage;
}

void FindPanel::UpdateLocationButton() {
  if (location_ == kCurrentPage) {
    SetButtonBitmaps(location_button_, IMAGE_LOCATION_PAGE);
    location_button_->SetToolTip(kTrCurrentPage);
  } else if (location_ == kAllPages) {
    SetButtonBitmaps(location_button_, IMAGE_LOCATION_ALL_PAGES);
    location_button_->SetToolTip(kTrAllPages);
  } else {
    SetButtonBitmaps(location_button_, IMAGE_LOCATION_FOLDERS);
    location_button_->SetToolTip(kTrFolder);
  }
}

bool FindPanel::IsRegexValid(const std::wstring& re_str) const {
  try {
    std::wregex re(re_str);
    return true;
  } catch (std::regex_error&) {
    return false;
  }
}

bool FindPanel::IsFindStringValid(const wxString& find_wxstr, bool empty_as_valid) const {
  if (find_wxstr.IsEmpty()) {
    return empty_as_valid;
  }

  if (GetBit(flags_, kFind_UseRegex)) {
    return IsRegexValid(find_wxstr.ToStdWstring());
  }

  return true;
}

void FindPanel::SetFindTextCtrlFgColor(bool valid) {
  wxColor color = valid ? fg_color_ : invalid_fg_color_;
  if (color != find_text_ctrl_->GetForegroundColour()) {
    find_text_ctrl_->SetForegroundColour(color);
    find_text_ctrl_->Refresh();
  }
}

wxString FindPanel::GetFolder() const {
  wxString folder = folder_text_ctrl_->GetValue();
  folder.Trim(true).Trim(false);
  return folder;
}

// TODO: If a folder includes another folder...
void FindPanel::AddFolder(const wxString& folder) {
  wxString folders_str = folder_text_ctrl_->GetValue();
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

  folder_text_ctrl_->SetValue(folders_str);
}

void FindPanel::AddFindString(const wxString& string) {
  session_->AddFindString(string);
}

void FindPanel::AddReplaceString(const wxString& string) {
  session_->AddReplaceString(string);
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
  wxSizer* vsizer = GetSizer();
  vsizer->Clear(false);

  wxSizer* head_vsizer = CommonLayoutHead(with_replace);
  wxSizer* body_vsizer = CommonLayoutBody(with_replace);
  wxSizer* foot_vsizer = CommonLayoutFoot(with_replace);

  wxSizer* ctrl_hsizer = new wxBoxSizer(wxHORIZONTAL);
  ctrl_hsizer->Add(head_vsizer, 0, wxEXPAND);
  ctrl_hsizer->Add(body_vsizer, 1, wxEXPAND | wxLEFT, kPaddingX);
  ctrl_hsizer->Add(foot_vsizer, 0, wxEXPAND | wxLEFT, kPaddingX);

  vsizer->AddSpacer(2);  // Top borders
  vsizer->Add(ctrl_hsizer, 0, wxEXPAND | wxALL, kPaddingX);

  Layout();
}

wxSizer* FindPanel::CommonLayoutHead(bool with_replace) {
  wxSizer* head_vsizer = new wxBoxSizer(wxVERTICAL);

  if (location_ == kFolders) {
    wxSizer* location_head_hsizer = new wxBoxSizer(wxHORIZONTAL);
    location_head_hsizer->AddStretchSpacer(1);  // Make the label right-aligned.
    location_head_hsizer->Add(folder_label_, 0, wxALIGN_CV);
    head_vsizer->Add(location_head_hsizer, 1, wxEXPAND);
    head_vsizer->AddSpacer(kPaddingY);
  }

  wxSizer* find_head_hsizer = new wxBoxSizer(wxHORIZONTAL);

  int cw = GetCharWidth();

  find_head_hsizer->Add(location_button_, 0, wxALIGN_CV);
  find_head_hsizer->Add(use_regex_tbutton_, 0, wxALIGN_CV | wxLEFT, cw);
  find_head_hsizer->Add(case_sensitive_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);
  find_head_hsizer->Add(match_word_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);
  find_head_hsizer->Add(reversely_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);

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
    location_body_hsizer->Add(folder_text_ctrl_, 1, wxALIGN_CV);
    body_vsizer->Add(location_body_hsizer, 1, wxEXPAND);
    body_vsizer->AddSpacer(kPaddingY);
  }

  wxSizer* find_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
  find_body_hsizer->Add(find_text_ctrl_, 1, wxALIGN_CV);
  body_vsizer->Add(find_body_hsizer, 1, wxEXPAND);

  if (with_replace) {
    body_vsizer->AddSpacer(kPaddingY);
    wxSizer* replace_body_hsizer = new wxBoxSizer(wxHORIZONTAL);
    replace_body_hsizer->Add(replace_text_ctrl_, 1, wxALIGN_CV);
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
    find_foot_hsizer->Add(find_button_, 1, wxALIGN_CV);
    find_foot_hsizer->Add(find_all_button_, 1, wxALIGN_CV | wxLEFT, kPaddingX);
  } else {
    find_foot_hsizer->Add(find_all_button_, 1, wxALIGN_CV);
  }
  foot_vsizer->Add(find_foot_hsizer, 1, wxEXPAND);

  if (with_replace) {
    foot_vsizer->AddSpacer(kPaddingY);
    wxSizer* replace_foot_hsizer = new wxBoxSizer(wxHORIZONTAL);
    if (replace_button_->IsShown()) {
      replace_foot_hsizer->Add(replace_button_, 1, wxALIGN_CV);
      replace_foot_hsizer->Add(replace_all_button_, 1, wxALIGN_CV | wxLEFT, kPaddingX);
    } else {
      replace_foot_hsizer->Add(replace_all_button_, 1, wxALIGN_CV);
    }
    foot_vsizer->Add(replace_foot_hsizer, 1, wxEXPAND);
  }

  return foot_vsizer;
}

void FindPanel::ShowReplace(bool show) {
  replace_text_ctrl_->Show(show);
  replace_button_->Show(show && location_ == kCurrentPage);
  replace_all_button_->Show(show);
}

void FindPanel::ShowFolders(bool show) {
  folder_label_->Show(show);
  folder_text_ctrl_->Show(show);
  add_folder_button_->Show(show);
}

void FindPanel::EnableButtons(bool enable) {
  find_button_->Enable(enable);
  find_all_button_->Enable(enable);
  replace_button_->Enable(enable);
  replace_all_button_->Enable(enable);
}

ui::BitmapButton* FindPanel::NewBitmapButton(int id, bool draw_bg, bool draw_border) {
  ui::BitmapButton* button = new ui::BitmapButton(button_style_);
  button->Create(this, id);
  button->set_user_best_size(bitmap_button_best_size_);
  button->set_draw_bg(draw_bg);
  button->set_draw_border(draw_border);
  return button;
}

ui::BitmapToggleButton* FindPanel::NewBitmapToggleButton(int id, bool draw_bg, bool draw_border) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(button_style_);
  button->Create(this, id);
  button->set_user_best_size(bitmap_button_best_size_);
  button->set_draw_bg(draw_bg);
  button->set_draw_border(draw_border);
  return button;
}

ui::TextButton* FindPanel::NewTextButton(int id, const wxString& label) {
  ui::TextButton* button = new ui::TextButton(button_style_);
  button->Create(this, id, label);
  return button;
}

void FindPanel::SetButtonBitmaps(ui::BitmapButtonBase* button, ImageId normal_id) {
  button->SetBitmaps(theme_->GetImage(normal_id));
}

void FindPanel::SetButtonBitmapsNH(ui::BitmapButtonBase* button,
                                   ImageId normal_id,
                                   ImageId hover_id) {
  wxBitmap normal = theme_->GetImage(normal_id);
  wxBitmap hover = theme_->GetImage(hover_id);

  button->SetBitmaps(normal, hover, hover, normal);
}

void FindPanel::SetButtonBitmapsND(ui::BitmapButtonBase* button,
                                   ImageId normal_id,
                                   ImageId disabled_id) {
  button->SetBitmapsND(theme_->GetImage(normal_id),
                       theme_->GetImage(disabled_id));
}

void FindPanel::SetButtonBitmapsNHD(ui::BitmapButtonBase* button,
                                    ImageId normal_id,
                                    ImageId hover_id,
                                    ImageId disabled_id) {
  wxBitmap hover = theme_->GetImage(hover_id);

  button->SetBitmaps(theme_->GetImage(normal_id),
                     hover,
                     hover,
                     theme_->GetImage(disabled_id));
}

void FindPanel::PostLayoutEvent() {
  wxCommandEvent evt(kFindPanelLayoutEvent, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace jil
