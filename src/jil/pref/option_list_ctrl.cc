#include "jil/pref/option_list_ctrl.h"
#include "wx/dcbuffer.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/settings.h"
#include "wx/textctrl.h"
#include "ui/util.h"

namespace jil {
namespace pref {

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(OlcHeadPanel, wxPanel)
EVT_PAINT(OlcHeadPanel::OnPaint)
EVT_MOUSE_EVENTS(OlcHeadPanel::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST(OlcHeadPanel::OnMouseCaptureLost)
END_EVENT_TABLE()

OlcHeadPanel::OlcHeadPanel(OptionListCtrl* option_list_ctrl, wxWindowID id)
    : wxPanel(option_list_ctrl, id)
    , option_list_ctrl_(option_list_ctrl) {
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

OlcHeadPanel::~OlcHeadPanel() {
}

void OlcHeadPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
  option_list_ctrl_->OnHeadPaint(dc);
}

void OlcHeadPanel::OnMouseEvents(wxMouseEvent& evt) {
  evt.Skip();
  option_list_ctrl_->OnHeadMouse(evt);
}

void OlcHeadPanel::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(OlcBodyPanel, wxPanel)
EVT_SIZE(OlcBodyPanel::OnSize)
EVT_PAINT(OlcBodyPanel::OnPaint)
EVT_LEFT_DOWN(OlcBodyPanel::OnMouseLeftDown)
EVT_SET_FOCUS(OlcBodyPanel::OnFocusChange)
EVT_KILL_FOCUS(OlcBodyPanel::OnFocusChange)
END_EVENT_TABLE()

OlcBodyPanel::OlcBodyPanel(OptionListCtrl* option_list_ctrl, wxWindowID id)
    : wxPanel(option_list_ctrl, id)
    , option_list_ctrl_(option_list_ctrl) {
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

OlcBodyPanel::~OlcBodyPanel() {
}

void OlcBodyPanel::OnSize(wxSizeEvent& evt) {
  option_list_ctrl_->OnBodySize(evt);
  evt.Skip();
}

void OlcBodyPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();

  option_list_ctrl_->OnBodyPaint(dc);
}

void OlcBodyPanel::OnMouseLeftDown(wxMouseEvent& evt) {
  // Skip so that the focus can be well handled by wxWidgets.
  evt.Skip();
  option_list_ctrl_->OnBodyMouseLeftDown(evt);
}

void OlcBodyPanel::OnFocusChange(wxFocusEvent& evt) {
  evt.Skip();
  option_list_ctrl_->OnBodyFocusChange(evt);
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(OptionListCtrl, wxScrolledWindow)
EVT_SIZE(OptionListCtrl::OnSize)
END_EVENT_TABLE()

OptionListCtrl::OptionListCtrl() {
  Init();
}

OptionListCtrl::~OptionListCtrl() {
  for (size_t i = 0; i < options_.size(); ++i) {
    delete options_[i];
  }
  options_.clear();
}

bool OptionListCtrl::Create(wxWindow* parent, int id, const wxSize& size, bool with_head, long style) {
  if (!wxScrolledWindow::Create(parent, id, wxDefaultPosition, size, style)) {
    return false;
  }

  SetBackgroundColour(parent->GetBackgroundColour());
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  SetWindowStyleFlag(wxBORDER_STATIC);

  InitColors();

  row_height_ = row_padding_.y + GetCharHeight() + row_padding_.y + 1;  // + 1 for row bar

  if (with_head) {
    head_panel_ = new OlcHeadPanel(this, wxID_ANY);
    head_panel_->SetBackgroundColour(GetColor(COLOR_HEAD_BG));
  }

  body_panel_ = new OlcBodyPanel(this, wxID_ANY);
  body_panel_->SetBackgroundColour(GetColor(COLOR_BODY_BG));

  SetTargetWindow(body_panel_);

  SetScrollbars(GetCharWidth(), row_height_, 1, 1);

  created_ = true;
  return true;
}

void OptionListCtrl::StartBatch() {
  batch_ = true;
}

void OptionListCtrl::EndBatch() {
  batch_ = false;
  Layout();
  Refresh();
}

void OptionListCtrl::AddOption(const editor::OptionPair& option_pair) {
  options_.push_back(new editor::OptionPair(option_pair));

  if (created_ && !batch_) {
    UpdateLayout();
    Refresh();
  }
}

const editor::OptionPair* OptionListCtrl::GetOption(int i) const {
  assert(i >= 0 && i < GetCount());
  return options_[i];
}

int OptionListCtrl::GetCount() const {
  return static_cast<int>(options_.size());
}

void OptionListCtrl::Init() {
  created_ = false;
  batch_ = false;

  head_panel_ = NULL;
  body_panel_ = NULL;

  row_padding_.Set(3, 3);
  row_height_ = 0;

  columns_[Column::KEY].label = _("Key");
  columns_[Column::VALUE].label = _("Value");

  selected_row_ = 0;

  text_ctrl_ = NULL;
}

void OptionListCtrl::InitColors() {
  SetColor(COLOR_HEAD_FG, wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT));
  SetColor(COLOR_HEAD_BG, wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION));
  SetColor(COLOR_HEAD_BORDER, wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVEBORDER));
  SetColor(COLOR_BODY_FG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
  SetColor(COLOR_BODY_FG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
  SetColor(COLOR_BODY_BG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
  SetColor(COLOR_BODY_BG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
  SetColor(COLOR_BODY_BG_HL_NOFOCUS, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
  SetColor(COLOR_BODY_BORDER, wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVEBORDER));
}

editor::OptionPair* OptionListCtrl::GetOptionByRow(int row) {
  assert(row > 0);
  size_t i = row - 1;
  if (i < options_.size()) {
    return options_[i];
  }
  return NULL;
}

void OptionListCtrl::OnSize(wxSizeEvent& evt) {
  if (m_targetWindow == this) {
    return;
  }

  UpdateLayout();
  Refresh();
}

void OptionListCtrl::OnEditingDone(wxCommandEvent& evt) {
  assert(text_ctrl_ != NULL);
  FinishEditing();
}

void OptionListCtrl::OnHeadPaint(wxDC& dc) {
  wxPen pen(GetColor(COLOR_HEAD_BORDER));
  wxBrush brush(GetColor(COLOR_HEAD_BG));

  dc.SetFont(head_panel_->GetFont());
  dc.SetTextForeground(GetColor(COLOR_HEAD_FG));
  dc.SetPen(pen);
  dc.SetBackground(brush);
  dc.Clear();

  const wxRect rect = head_panel_->GetClientRect();

  int x = rect.x;

  for (size_t i = 0; i < Column::COUNT; ++i) {
    Column& column = columns_[i];

    wxRect column_rect(x, rect.y, column.width, row_height_);
    column_rect.Deflate(row_padding_);
    ui::DrawTextInRect(dc, column.label, column_rect);

    x += column.width;

    // Column border.
    int col_x = x - 1;
    dc.DrawLine(col_x, rect.y, col_x, rect.GetBottom());
  }
}

// TODO: Resize columns
void OptionListCtrl::OnHeadMouse(wxMouseEvent& evt) {
}

void OptionListCtrl::OnBodySize(wxSizeEvent& evt) {
  int client_size = body_panel_->GetClientSize().x;
  int column_size = client_size / Column::COUNT;

  for (int i = 1; i < Column::COUNT; ++i) {
    columns_[i].width = column_size;
    client_size -= column_size;
  }

  columns_[0].width = client_size;
}

void OptionListCtrl::OnBodyPaint(wxDC& dc) {
  PrepareDC(dc);

  wxColour bg_hl_color = GetColor(COLOR_BODY_BG_HL);
  wxColour bg_hl_nofocus_color = GetColor(COLOR_BODY_BG_HL_NOFOCUS);
  wxColour fg_color = GetColor(COLOR_BODY_FG);
  wxColour fg_hl_color = GetColor(COLOR_BODY_FG_HL);

  dc.SetTextForeground(fg_color);
  dc.SetPen(GetColor(COLOR_BODY_BORDER));

  const wxRect rect = body_panel_->GetClientRect();
  int y = rect.y;

  int row_count = GetCount();

  for (int row = 1; row <= row_count; ++row) {
    editor::OptionPair* option_pair = GetOptionByRow(row);

    int x = rect.x;

    for (int col = 0; col < Column::COUNT; ++col) {
      Column& column = columns_[col];

      if (row == selected_row_) {
        wxPen pen = dc.GetPen();  // Backup
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(body_panel_->HasFocus() ? bg_hl_color : bg_hl_nofocus_color);

        dc.DrawRectangle(x, y, column.width, row_height_ - 1);

        dc.SetPen(pen);  // Restore
      }

      wxString text;
      if (col == Column::KEY) {
        text = option_pair->key;
      } else if (col == Column::VALUE) {
        text = option_pair->value.ToString();
      }

      if (row == selected_row_) {
        dc.SetTextForeground(fg_hl_color);
      }

      wxRect text_rect(x, y, column.width, row_height_);
      text_rect.Deflate(row_padding_);
      ui::DrawTextInRect(dc, text, text_rect);

      if (row == selected_row_) {
        dc.SetTextForeground(fg_color);  // Restore
      }

      x += column.width;

      // Column border
      int col_x = x - 1;
      dc.DrawLine(col_x, y, col_x, y + row_height_);
    }

    y += row_height_;

    // Row border
    int row_y = y - 1;
    dc.DrawLine(rect.x, row_y, rect.GetRight(), row_y);
  }
}

void OptionListCtrl::OnBodyMouseLeftDown(wxMouseEvent& evt) {
  wxPoint pos = evt.GetPosition();

  int unscrolled_x = 0;
  int unscrolled_y = 0;
  CalcUnscrolledPosition(pos.x, pos.y, &unscrolled_x, &unscrolled_y);

  int row = (unscrolled_y + row_height_) / row_height_;
  if (row > GetCount()) {
    // Click on blank area.
    row = 0;
  }

  int old_selected_row = selected_row_;
  selected_row_ = row;

  if (selected_row_ == old_selected_row) {
    if (selected_row_ != 0) {
      if (!IsEditing()) {
        StartEditing(selected_row_);
      }
    }
  } else {
    if (old_selected_row != 0) {
      if (IsEditing()) {
        FinishEditing();
      }
      RefreshRow(old_selected_row);
    }

    if (selected_row_ != 0) {
      RefreshRow(selected_row_);
    }
  }
}

void OptionListCtrl::OnBodyFocusChange(wxFocusEvent& evt) {
  if (selected_row_ != 0) {
    RefreshRow(selected_row_);
  }
}

bool OptionListCtrl::IsEditing() const {
  return (text_ctrl_ != NULL && text_ctrl_->IsShown());
}

void OptionListCtrl::StartEditing(int row) {
  editor::OptionPair* option_pair = GetOptionByRow(row);
  wxRect rect = GetColumnRect(row, Column::VALUE);

  if (text_ctrl_ == NULL) {
    text_ctrl_ = new wxTextCtrl(body_panel_, wxID_ANY, option_pair->value.ToString(),
                                rect.GetPosition(), rect.GetSize(), wxTE_PROCESS_ENTER);
    Connect(text_ctrl_->GetId(),
            wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(OptionListCtrl::OnEditingDone));
  } else {
    text_ctrl_->SetValue(option_pair->value.ToString());
    text_ctrl_->SetSize(rect);
    text_ctrl_->Show();
  }

  // Save the option pair in client data so that OnEditingDone can use it.
  text_ctrl_->SetClientData(option_pair);

  text_ctrl_->SelectAll();
  text_ctrl_->SetFocus();
}

void OptionListCtrl::FinishEditing() {
  void* client_data = text_ctrl_->GetClientData();
  if (client_data == NULL) {
    return;
  }

  editor::OptionPair* option_pair = static_cast<editor::OptionPair*>(client_data);
  int value_type = option_pair->value.type();

  wxString value_str = text_ctrl_->GetValue();

  if (value_type == editor::OptionValue::kBool) {
    value_str.Trim(true).Trim(false);
  }

  if (option_pair->value.Parse(value_str)) {
    RefreshRowByKey(option_pair->key);
  } else {
    wxLogDebug(wxT("Invalid option value: %s"), value_str);
  }

  if (text_ctrl_ != NULL) {
    text_ctrl_->Hide();
  }
}

wxRect OptionListCtrl::GetRowClientRect(int row) const {
  if (row < 1 || row > GetCount()) {
    return wxRect();
  }

  int y = row_height_ * (row - 1);
  int scrolled_y = 0;
  CalcScrolledPosition(0, y, NULL, &scrolled_y);

  wxRect client_rect = body_panel_->GetClientRect();
  return wxRect(client_rect.x, scrolled_y, client_rect.GetRight(), row_height_);
}

void OptionListCtrl::RefreshRow(int row) {
  body_panel_->RefreshRect(GetRowClientRect(row));
}

void OptionListCtrl::RefreshRowByKey(const std::string& key) {
  for (size_t i = 0; i < options_.size(); ++i) {
    if (options_[i]->key == key) {
      int row = static_cast<int>(i) + 1;
      RefreshRow(row);
      break;
    }
  }
}

int OptionListCtrl::GetColumnX(int col) const {
  int x = 0;
  for (int i = 0; i < col; ++i) {
    x += GetColumnWidth(i);
  }
  return x;
}

int OptionListCtrl::GetColumnWidth(int col) const {
  return columns_[col].width;
}

wxRect OptionListCtrl::GetColumnRect(int row, int col) const {
  wxRect body_rect = body_panel_->GetClientRect();
  int x = GetColumnX(col);
  int y = GetScrolledY(body_rect.y + row_height_ * (row - 1));
  return wxRect(x, y, GetColumnWidth(col), row_height_);
}

int OptionListCtrl::GetScrolledX(int unscrolled_x) const {
  int x = 0;
  CalcScrolledPosition(unscrolled_x, 0, &x, NULL);
  return x;
}

int OptionListCtrl::GetScrolledY(int unscrolled_y) const {
  int y = 0;
  CalcScrolledPosition(0, unscrolled_y, NULL, &y);
  return y;
}

void OptionListCtrl::UpdateLayout() {
  const wxRect rect = GetClientRect();

  int y = rect.y;
  int w = rect.width + 1;
  int h = rect.height;

  if (head_panel_ != NULL) {
    head_panel_->SetSize(rect.x, y, w, row_height_);
    y += row_height_;
    h -= row_height_;
  }

  body_panel_->SetSize(rect.x, y, w, h);

  UpdateVirtualSize();

  AdjustScrollbars();
  Layout();
}

void OptionListCtrl::UpdateVirtualSize() {
  wxSize vsize;
  vsize.x = -1;
  vsize.y = row_height_ * GetCount();

  // Set virtual size for body panel since it's the target window.
  body_panel_->SetVirtualSize(vsize);
}

}  // namespace pref
}  // namespace jil
