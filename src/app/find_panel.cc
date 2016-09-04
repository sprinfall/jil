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
#include "ui/separator.h"
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
#define kTrOptions            _("Options")

#define kTrLocation           _("Location")
#define kTrUseRegex           _("Use regular expression")
#define kTrCaseSensitive      _("Case sensitive")
#define kTrMatchWord          _("Match whole word")

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

class TextCtrl : public wxTextCtrl {
  DECLARE_DYNAMIC_CLASS(TextCtrl)
  DECLARE_NO_COPY_CLASS(TextCtrl)
  DECLARE_EVENT_TABLE()

public:
  TextCtrl() {
  }

  TextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString)
      : wxTextCtrl(parent, id, value) {
    SetWindowStyleFlag(wxTE_PROCESS_ENTER);
    SetLayoutDirection(wxLayout_LeftToRight);
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

IMPLEMENT_DYNAMIC_CLASS(TextCtrl, wxTextCtrl)

BEGIN_EVENT_TABLE(TextCtrl, wxTextCtrl)
EVT_KEY_UP(TextCtrl::OnKeyUp)
END_EVENT_TABLE();

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC_CLASS(FindPanel, wxPanel);

BEGIN_EVENT_TABLE(FindPanel, wxPanel)
EVT_PAINT         (FindPanel::OnPaint)

EVT_MENU          (ID_FP_MENU_CURRENT_PAGE,       FindPanel::OnMenuCurrentPage)
EVT_MENU          (ID_FP_MENU_ALL_PAGES,          FindPanel::OnMenuAllPages)
EVT_MENU          (ID_FP_MENU_FOLDERS,            FindPanel::OnMenuFolders)

EVT_BUTTON        (ID_FP_ADD_FOLDER_BUTTON,       FindPanel::OnAddFolderButtonClick)

#if JIL_BMP_BUTTON_FIND_OPTIONS

EVT_BUTTON        (ID_FP_LOCATION_BUTTON,         FindPanel::OnLocationClick)

EVT_TOGGLEBUTTON  (ID_FP_USE_REGEX_TBUTTON,       FindPanel::OnUseRegexToggle)
EVT_TOGGLEBUTTON  (ID_FP_CASE_SENSITIVE_TBUTTON,  FindPanel::OnCaseSensitiveToggle)
EVT_TOGGLEBUTTON  (ID_FP_MATCH_WORD_TBUTTON,      FindPanel::OnMatchWordToggle)

#else

EVT_BUTTON        (ID_FP_LOCATION_LABEL,          FindPanel::OnLocationClick)  // TODO
EVT_BUTTON        (ID_FP_OPTIONS_LABEL,           FindPanel::OnOptionsLabel)

EVT_MENU          (ID_FP_MENU_USE_REGEX,          FindPanel::OnMenuUseRegex)
EVT_MENU          (ID_FP_MENU_CASE_SENSITIVE,     FindPanel::OnMenuCaseSensitive)
EVT_MENU          (ID_FP_MENU_MATCH_WORD,         FindPanel::OnMenuMatchWord)

#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

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

  button_style_.reset(new ui::ButtonStyle);
  InitButtonStyle();

  //----------------------------------------------------------------------------

  folders_bitmap_ = new wxStaticBitmap(this, ID_FP_FOLDERS_LABEL, wxNullBitmap);
  folders_bitmap_->Hide();

  folders_text_ctrl_ = new wxTextCtrl(this, ID_FP_FOLDERS_TEXTCTRL);
  folders_text_ctrl_->Hide();

  int height = folders_text_ctrl_->GetBestSize().GetHeight();
  bitmap_button_best_size_.Set(height, height);

  add_folder_button_ = NewBitmapButton(ID_FP_ADD_FOLDER_BUTTON);
  add_folder_button_->Hide();

  //----------------------------------------------------------------------------

#if JIL_BMP_BUTTON_FIND_OPTIONS

  location_button_ = NewBitmapButton(ID_FP_LOCATION_BUTTON);
  location_button_->SetToolTip(kTrLocation);

  use_regex_tbutton_ = NewBitmapToggleButton(ID_FP_USE_REGEX_TBUTTON);
  use_regex_tbutton_->SetToolTip(kTrUseRegex);

  case_sensitive_tbutton_ = NewBitmapToggleButton(ID_FP_CASE_SENSITIVE_TBUTTON);
  case_sensitive_tbutton_->SetToolTip(kTrCaseSensitive);

  match_word_tbutton_ = NewBitmapToggleButton(ID_FP_MATCH_WORD_TBUTTON);
  match_word_tbutton_->SetToolTip(kTrMatchWord);

  // Initialize toggle button states.
  use_regex_tbutton_->set_toggle(GetBit(flags_, kFind_UseRegex));
  case_sensitive_tbutton_->set_toggle(GetBit(flags_, kFind_CaseSensitive));
  match_word_tbutton_->set_toggle(GetBit(flags_, kFind_MatchWord));

#else

  location_label_ = new ui::Label(this, ID_FP_LOCATION_LABEL, kTrLocation);
  location_label_->SetCursor(wxCursor(wxCURSOR_HAND));

  sep_ = new ui::Separator(this, 1, false);

  options_label_ = new ui::Label(this, ID_FP_OPTIONS_LABEL, kTrOptions);
  options_label_->SetCursor(wxCursor(wxCURSOR_HAND));

#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

  //------------------------------------

  find_text_ctrl_ = new TextCtrl(this, ID_FP_FIND_TEXTCTRL);
  replace_text_ctrl_ = new TextCtrl(this, ID_FP_REPLACE_TEXTCTRL);

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

  return true;
}

FindPanel::~FindPanel() {
}

bool FindPanel::Destroy() {
  session_->set_find_flags(flags_);
  session_->set_find_location(location_);
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

  find_text_ctrl_->SetFocus();
  if (!find_text_ctrl_->GetValue().IsEmpty()) {
    find_text_ctrl_->SelectAll();
  }
}

bool FindPanel::SetFont(const wxFont& font) {
  bool result = wxPanel::SetFont(font);

  wxWindowList& children = wxWindow::GetChildren(); 
  for (size_t i = 0; i < children.GetCount(); ++i) {
    children[i]->SetFont(font);
    //children[i]->InvalidateBestSize();  // Seems not necessary.
    children[i]->Refresh();  // !
  }

  Layout();

  return result;
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

  // Add current working dir.
  if (folders_text_ctrl_->GetValue().IsEmpty()) {
    wxString cwd = wxGetCwd();
    folders_text_ctrl_->SetValue(cwd);
  }
}

void FindPanel::SetFindString(const wxString& find_string) {
  find_text_ctrl_->SetValue(find_string);
  find_text_ctrl_->SelectAll();
  AddFindString(find_string);
}

void FindPanel::ReapplyTheme() {
  assert(theme_);

  InitButtonStyle();

  folders_bitmap_->SetBitmap(theme_->GetImage(IMAGE_FOLDERS));
  folders_bitmap_->SetForegroundColour(theme_->GetColor(COLOR_FG));

  add_folder_button_->SetBitmap(theme_->GetImage(IMAGE_ADD_FOLDER));

#if JIL_BMP_BUTTON_FIND_OPTIONS
  location_button_->SetBitmap(theme_->GetImage(IMAGE_LOCATION));
  use_regex_tbutton_->SetBitmap(theme_->GetImage(IMAGE_USE_REGEX));
  case_sensitive_tbutton_->SetBitmap(theme_->GetImage(IMAGE_CASE_SENSITIVE));
  match_word_tbutton_->SetBitmap(theme_->GetImage(IMAGE_MATCH_WORD));
#else
  location_label_->SetForegroundColour(theme_->GetColor(COLOR_FG));
  sep_->SetColor(theme_->GetColor(COLOR_FG));
  options_label_->SetForegroundColour(theme_->GetColor(COLOR_FG));
#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

}

void FindPanel::OnAddFolderButtonClick(wxCommandEvent& evt) {
  wxString default_path = wxGetCwd();
  long style = wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST;
  wxDirDialog dlg(this, wxDirSelectorPromptStr, default_path, style);

  if (dlg.ShowModal() == wxID_OK) {
    AddFolder(dlg.GetPath());
  }
}

void FindPanel::OnLocationClick(wxCommandEvent& evt) {
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

#if JIL_BMP_BUTTON_FIND_OPTIONS

void FindPanel::OnUseRegexToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_UseRegex, evt.IsChecked());
}

void FindPanel::OnCaseSensitiveToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_CaseSensitive, evt.IsChecked());
}

void FindPanel::OnMatchWordToggle(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_MatchWord, evt.IsChecked());
}

#else

void FindPanel::OnOptionsLabel(wxCommandEvent& evt) {
  wxMenu menu;

  menu.AppendCheckItem(ID_FP_MENU_USE_REGEX, kTrUseRegex);
  menu.AppendCheckItem(ID_FP_MENU_CASE_SENSITIVE, kTrCaseSensitive);
  menu.AppendCheckItem(ID_FP_MENU_MATCH_WORD, kTrMatchWord);

  if (GetBit(flags_, kFind_UseRegex)) {
    menu.Check(ID_FP_MENU_USE_REGEX, true);
  }
  if (GetBit(flags_, kFind_CaseSensitive)) {
    menu.Check(ID_FP_MENU_CASE_SENSITIVE, true);
  }
  if (GetBit(flags_, kFind_MatchWord)) {
    menu.Check(ID_FP_MENU_MATCH_WORD, true);
  }

  PopupMenu(&menu, ScreenToClient(wxGetMousePosition()));
}

void FindPanel::OnMenuUseRegex(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_UseRegex, evt.IsChecked());
}

void FindPanel::OnMenuCaseSensitive(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_CaseSensitive, evt.IsChecked());
}

void FindPanel::OnMenuMatchWord(wxCommandEvent& evt) {
  flags_ = SetBit(flags_, kFind_MatchWord, evt.IsChecked());
}

#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

void FindPanel::OnFind(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    HandleFind();
  }
}

void FindPanel::OnFindAll(wxCommandEvent& evt) {
  wxString find_str = find_text_ctrl_->GetValue();
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
    wxString find_str = find_text_ctrl_->GetValue();
    book_frame_->FindInActivePageIncrementally(find_str.ToStdWstring(), flags_);
  }
}

void FindPanel::OnFindTextEnter(wxCommandEvent& evt) {
  if (location_ == kCurrentPage) {
    HandleFind();
  }
}

void FindPanel::OnFindTextHistory(wxCommandEvent& evt) {
  const std::list<wxString>& find_strings = session_->find_strings();
  ShowHistoryMenu(find_strings, ID_MENU_FIND_HISTORY_BEGIN, find_text_ctrl_);
}

void FindPanel::OnReplaceTextHistory(wxCommandEvent& evt) {
  const std::list<wxString>& replace_strings = session_->replace_strings();
  ShowHistoryMenu(replace_strings, ID_MENU_REPLACE_HISTORY_BEGIN, replace_text_ctrl_);
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

  wxString find_str = find_text_ctrl_->GetValue();
  if (find_str.IsEmpty()) {
    return;
  }

  AddFindString(find_str);

  book_frame_->FindInActivePage(find_str.ToStdWstring(), flags_);
}

void FindPanel::HandleReplace(bool all) {
  wxString find_str = find_text_ctrl_->GetValue();
  if (!find_str.IsEmpty()) {
    AddFindString(find_str);

    wxString replace_str = replace_text_ctrl_->GetValue();
    if (!replace_str.IsEmpty()) {
      AddReplaceString(replace_str);
    }

    // TODO
  }
}

void FindPanel::SetLocation(FindLocation location) {
  if (location_ != location) {
    bool folders = location_ == kFolders || location == kFolders;
    location_ = location;

    if (folders) {
      ShowFolders(location == kFolders);
      PostLayoutEvent();
    }

    UpdateLayout();
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
    location_head_hsizer->Add(folders_bitmap_, 0, wxALIGN_CV);
    head_vsizer->Add(location_head_hsizer, 1, wxEXPAND);
    head_vsizer->AddSpacer(kPaddingY);
  }

  wxSizer* find_head_hsizer = new wxBoxSizer(wxHORIZONTAL);

  int cw = GetCharWidth();

#if JIL_BMP_BUTTON_FIND_OPTIONS
  find_head_hsizer->Add(location_button_, 0, wxALIGN_CV);
  find_head_hsizer->Add(use_regex_tbutton_, 0, wxALIGN_CV | wxLEFT, cw);
  find_head_hsizer->Add(case_sensitive_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);
  find_head_hsizer->Add(match_word_tbutton_, 0, wxALIGN_CV | wxLEFT, 2);
#else
  find_head_hsizer->Add(location_label_, 0, wxALIGN_CV);
  find_head_hsizer->AddSpacer(cw);
  find_head_hsizer->Add(sep_, 0, wxEXPAND | wxTB, 7);
  find_head_hsizer->Add(options_label_, 0, wxALIGN_CV | wxLEFT, cw);
#endif  // JIL_BMP_BUTTON_FIND_OPTIONS

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
    //replace_foot_hsizer->Add(replace_history_button_, 0, wxALIGN_CV | wxRIGHT, kPaddingX);
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
  //replace_history_button_->Show(show);
  replace_button_->Show(show && location_ == kCurrentPage);
  replace_all_button_->Show(show);
}

void FindPanel::ShowFolders(bool show) {
  folders_bitmap_->Show(show);
  folders_text_ctrl_->Show(show);
  add_folder_button_->Show(show);
}

void FindPanel::InitButtonStyle() {
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

ui::BitmapButton* FindPanel::NewBitmapButton(int id) {
  ui::BitmapButton* button = new ui::BitmapButton(button_style_);
  button->Create(this, id);
  button->set_user_best_size(bitmap_button_best_size_);
  return button;
}

ui::BitmapToggleButton* FindPanel::NewBitmapToggleButton(int id) {
  ui::BitmapToggleButton* button = new ui::BitmapToggleButton(button_style_);
  button->Create(this, id);
  button->set_user_best_size(bitmap_button_best_size_);
  return button;
}

ui::TextButton* FindPanel::NewTextButton(int id, const wxString& label) {
  ui::TextButton* button = new ui::TextButton(button_style_);
  button->Create(this, id, label);
  return button;
}

void FindPanel::PostLayoutEvent() {
  wxCommandEvent evt(kFindPanelLayoutEvent, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace jil
