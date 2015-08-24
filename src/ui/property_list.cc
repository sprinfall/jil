#include "ui/property_list.h"
#include "wx/dcbuffer.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/textctrl.h"
#include "ui/util.h"

namespace jil {
namespace ui {

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(HeadPanel, wxPanel)
EVT_PAINT(HeadPanel::OnPaint)
EVT_MOUSE_EVENTS(HeadPanel::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST(HeadPanel::OnMouseCaptureLost)
END_EVENT_TABLE()

HeadPanel::HeadPanel(PropertyList* property_ctrl, wxWindowID id)
    : wxPanel(property_ctrl, id), property_ctrl_(property_ctrl) {
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);

  wxFont font = GetFont();
  font.SetWeight(wxFONTWEIGHT_BOLD);
  SetFont(font);
}

HeadPanel::~HeadPanel() {
}

// TODO
void HeadPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);

  //// Don't call property_ctrl_->PrepareDC(dc).
  //int unscrolled_x = 0;
  //property_ctrl_->CalcUnscrolledPosition(0, 0, &unscrolled_x, NULL);

  //wxPoint device_origin = dc.GetDeviceOrigin();
  //if (GetLayoutDirection() == wxLayout_RightToLeft) {
  //  dc.SetDeviceOrigin(device_origin.x + unscrolled_x, device_origin.y);
  //} else {
  //  dc.SetDeviceOrigin(device_origin.x - unscrolled_x, device_origin.y);
  //}

  property_ctrl_->OnHeadPaint(dc);
}

void HeadPanel::OnMouseEvents(wxMouseEvent& evt) {
  property_ctrl_->OnHeadMouse(evt);
}

void HeadPanel::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(BodyPanel, wxPanel)
EVT_PAINT       (BodyPanel::OnPaint)
EVT_LEFT_DOWN   (BodyPanel::OnMouseLeftDown)
EVT_SET_FOCUS   (BodyPanel::OnSetFocus)
EVT_KILL_FOCUS  (BodyPanel::OnKillFocus)
END_EVENT_TABLE()

BodyPanel::BodyPanel(PropertyList* property_ctrl, wxWindowID id)
    : wxPanel(property_ctrl, id), property_ctrl_(property_ctrl) {
  SetBackgroundColour(GetParent()->GetBackgroundColour());
  SetBackgroundStyle(wxBG_STYLE_PAINT);
}

BodyPanel::~BodyPanel() {
}

void BodyPanel::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear(); // Color the background!

  property_ctrl_->OnBodyPaint(dc);
}

void BodyPanel::OnMouseLeftDown(wxMouseEvent& evt) {
  // Skip so that the focus can be well handled by wxWidgets.
  evt.Skip();
  property_ctrl_->OnBodyMouseLeftDown(evt);
}

void BodyPanel::OnSetFocus(wxFocusEvent& evt) {
  evt.Skip();
  property_ctrl_->OnBodySetFocus(evt);
}

void BodyPanel::OnKillFocus(wxFocusEvent& evt) {
  evt.Skip();
  property_ctrl_->OnBodyKillFocus(evt);
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(PropertyList, wxScrolledWindow)
EVT_SIZE        (PropertyList::OnSize)
END_EVENT_TABLE()

PropertyList::PropertyList() {
  Init();
}

PropertyList::~PropertyList() {
  for (size_t i = 0; i < properties_.size(); ++i) {
    delete properties_[i];
  }
  properties_.clear();
}

bool PropertyList::Create(wxWindow* parent, int id, const wxSize& size, long style) {
  if (!wxScrolledWindow::Create(parent, id, wxDefaultPosition, size, style)) {
    return false;
  }

  SetBackgroundColour(parent->GetBackgroundColour());
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  SetWindowStyleFlag(wxBORDER_SIMPLE);

  CheckColors();

  row_height_ = row_padding_.y + GetCharHeight() + row_padding_.y + 1;  // + 1 for row bar

  head_panel_ = new HeadPanel(this, wxID_ANY);
  head_panel_->SetBackgroundColour(GetColor(COLOR_HEAD_BG));

  body_panel_ = new BodyPanel(this, wxID_ANY);
  body_panel_->SetBackgroundColour(GetColor(COLOR_BODY_BG));

  SetTargetWindow(body_panel_);

  SetScrollbars(GetCharWidth(), row_height_, 1, 1);

  created_ = true;
  return true;
}

void PropertyList::StartBatch() {
  batch_ = true;
}

void PropertyList::EndBatch() {
  batch_ = false;
  Layout();
  Refresh();
}

void PropertyList::AddProperty(const wxString& key, const wxString& value) {
  Property* prop = new Property;
  prop->key = key;
  prop->value = value;
  properties_.push_back(prop);

  if (created_ && !batch_) {
    UpdateLayout();
    Refresh();
  }
}

void PropertyList::Init() {
  created_ = false;
  batch_ = false;

  row_padding_.Set(3, 3);
  row_height_ = 0;

  selected_row_ = 0;

  text_ctrl_ = NULL;
}

void PropertyList::CheckColors() {
  if (!GetColor(COLOR_HEAD_FG).IsOk()) {
    SetColor(COLOR_HEAD_FG, *wxWHITE);
  }

  if (!GetColor(COLOR_HEAD_BG).IsOk()) {
    SetColor(COLOR_HEAD_BG, wxColour(75, 75, 75));
  }

  if (!GetColor(COLOR_HEAD_BAR).IsOk()) {
    SetColor(COLOR_HEAD_BAR, *wxLIGHT_GREY);
  }

  if (!GetColor(COLOR_BODY_FG).IsOk()) {
    SetColor(COLOR_BODY_FG, *wxBLACK);
  }

  if (!GetColor(COLOR_BODY_BG).IsOk()) {
    SetColor(COLOR_BODY_BG, *wxWHITE);
  }

  if (!GetColor(COLOR_BODY_BG_SELECT).IsOk()) {
    SetColor(COLOR_BODY_BG_SELECT, wxColour(51, 153, 255));
  }
   
  if (!GetColor(COLOR_BODY_BAR).IsOk()) {
    SetColor(COLOR_BODY_BAR, *wxLIGHT_GREY);
  }
}

PropertyList::Property* PropertyList::GetPropertyByRow(int row) {
  assert(row > 0);
  size_t i = row - 1;
  if (i < properties_.size()) {
    return properties_[i];
  }
  return NULL;
}

void PropertyList::OnSize(wxSizeEvent& evt) {
  if (m_targetWindow == this) {
    return;
  }

  UpdateLayout();
  Refresh();
}

void PropertyList::OnEditingDone(wxCommandEvent& evt) {
  assert(text_ctrl_ != NULL);

  Property* prop = static_cast<Property*>(text_ctrl_->GetClientData());

  wxString value = text_ctrl_->GetValue();
  prop->value = value;
  wxLogDebug(prop->key + wxT(": ") + value);

  StopEditing();
}

struct Column {
  wxString label;
  int width;
};

void PropertyList::OnHeadPaint(wxDC& dc) {
  wxPen pen(GetColor(COLOR_HEAD_BAR));
  wxBrush brush(GetColor(COLOR_HEAD_BG));

  dc.SetFont(head_panel_->GetFont());
  dc.SetTextForeground(GetColor(COLOR_HEAD_FG));
  dc.SetPen(pen);
  dc.SetBackground(brush);
  dc.Clear();

  Column columns[2] = {
    { _("Key"), GetKeyWidth() },
    { _("Value"), GetValueWidth() },
  };

  const wxRect rect = head_panel_->GetClientRect();
  int x = rect.x;
  int y = rect.y;

  for (size_t i = 0; i < 2; ++i) {
    Column& column = columns[i];

    int label_w = 0;
    int label_h = 0;
    dc.GetTextExtent(column.label, &label_w, &label_h, NULL, NULL);

    int label_expected_w = column.width - row_padding_.x - row_padding_.x;

    wxString column_label = column.label;
    // The label is too long to display fully.
    if (label_w > label_expected_w) {
      column_label = column_label.Mid(0, TailorLabel(dc, column.label, label_expected_w));
      //dc.GetTextExtent(column_label, &label_w, NULL, NULL, NULL);
    }

    int label_x = x + row_padding_.x;
    dc.DrawText(column_label, label_x, y + row_padding_.y);

    x += column.width;

    // Column border.
    dc.DrawLine(x - 1, y + 1, x - 1, rect.GetBottom() - 1);
  }
}

void PropertyList::OnHeadMouse(wxMouseEvent& evt) {
}

void PropertyList::OnBodyPaint(wxDC& dc) {
  PrepareDC(dc);

  dc.SetPen(wxPen(GetColor(COLOR_BODY_BAR)));
  dc.SetTextForeground(GetColor(COLOR_BODY_FG));

  const wxRect rect = body_panel_->GetClientRect();
  int key_width = GetKeyWidth();

  int x = rect.x;
  int y = rect.y;

  int row_count = GetCount();

  for (int row = 1; row <= row_count; ++row) {
    if (row == selected_row_) {
      wxPen pen = dc.GetPen();
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(wxBrush(GetColor(COLOR_BODY_BG_SELECT)));

      wxRect select_rect(x, y, key_width, row_height_ - 1);
      dc.DrawRectangle(select_rect);

      dc.SetPen(pen);
    }

    Property* prop = GetPropertyByRow(row);

    dc.DrawText(prop->key, x + row_padding_.x, y + row_padding_.y);

    dc.DrawText(prop->value, x + key_width + row_padding_.x, y + row_padding_.y);

    // Column bar
    int col_bar_x = x + key_width - 1;
    dc.DrawLine(col_bar_x, y, col_bar_x, y + row_height_);

    y += row_height_;

    // Row bar
    int row_bar_y = y - 1;
    dc.DrawLine(x, row_bar_y, rect.GetRight(), row_bar_y);
  }
}

void PropertyList::OnBodyMouseLeftDown(wxMouseEvent& evt) {
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
        StopEditing();
      }
      RefreshRow(old_selected_row);
    }

    if (selected_row_ != 0) {
      RefreshRow(selected_row_);
    }
  }
}

void PropertyList::OnBodySetFocus(wxFocusEvent& evt) {
  if (selected_row_ != 0) {
    RefreshRow(selected_row_);
  }
}

void PropertyList::OnBodyKillFocus(wxFocusEvent& evt) {
  if (selected_row_ != 0) {
    RefreshRow(selected_row_);
  }
}

bool PropertyList::IsEditing() const {
  return (text_ctrl_ != NULL && text_ctrl_->IsShown());
}

void PropertyList::StartEditing(int row) {
  Property* prop = GetPropertyByRow(row);
  wxRect rect = GetValueRect(row);

  if (text_ctrl_ == NULL) {
    text_ctrl_ = new wxTextCtrl(body_panel_, wxID_ANY, prop->value,
                                rect.GetPosition(), rect.GetSize(), wxTE_PROCESS_ENTER);
    Connect(text_ctrl_->GetId(),
            wxEVT_COMMAND_TEXT_ENTER,
            wxCommandEventHandler(PropertyList::OnEditingDone));
  } else {
    text_ctrl_->SetValue(prop->value);
    text_ctrl_->SetSize(rect);
    text_ctrl_->Show();
  }

  text_ctrl_->SetClientData(prop);

  text_ctrl_->SetInsertionPointEnd();
  text_ctrl_->SetFocus();
}

void PropertyList::StopEditing() {
  if (text_ctrl_ != NULL) {
    text_ctrl_->Hide();
  }
}

wxRect PropertyList::GetRowClientRect(int row) const {
  if (row < 1 || row > GetCount()) {
    return wxRect();
  }

  int y = row_height_ * (row - 1);
  int scrolled_y = 0;
  CalcScrolledPosition(0, y, NULL, &scrolled_y);

  wxRect client_rect = body_panel_->GetClientRect();
  return wxRect(client_rect.x, scrolled_y, client_rect.GetRight(), row_height_);
}

void PropertyList::RefreshRow(int row) {
  body_panel_->RefreshRect(GetRowClientRect(row));
}

void PropertyList::UpdateLayout() {
  const wxRect rect = GetClientRect();

  int y = rect.y;

  head_panel_->SetSize(rect.x, y, rect.width + 1, row_height_);
  y += row_height_;

  body_panel_->SetSize(rect.x, y, rect.width + 1, rect.height - row_height_);

  UpdateVirtualSize();

  AdjustScrollbars();
  Layout();
}

void PropertyList::UpdateVirtualSize() {
  wxSize vsize;
  vsize.x = -1;
  vsize.y = row_height_ * GetCount();

  // Set virtual size for body panel since it's the target window.
  body_panel_->SetVirtualSize(vsize);
}

int PropertyList::GetKeyWidth() const {
  wxSize size = body_panel_->GetClientSize();
  return size.x / 2;
}

int PropertyList::GetValueWidth() const {
  wxSize size = body_panel_->GetClientSize();
  return size.x - GetKeyWidth();
}

wxRect PropertyList::GetKeyRect(int row) const {
  wxRect body_rect = body_panel_->GetClientRect();
  int y = GetScrolledY(body_rect.y + row_height_ * (row - 1));
  return wxRect(body_rect.x, y, GetKeyWidth(), row_height_);
}

wxRect PropertyList::GetValueRect(int row) const {
  wxRect body_rect = body_panel_->GetClientRect();
  int x = body_rect.x + GetKeyWidth();
  int y = GetScrolledY(body_rect.y + row_height_ * (row - 1));
  return wxRect(x, y, GetValueWidth(), row_height_);
}

int PropertyList::GetScrolledX(int unscrolled_x) const {
  int x = 0;
  CalcScrolledPosition(unscrolled_x, 0, &x, NULL);
  return x;
}

int PropertyList::GetScrolledY(int unscrolled_y) const {
  int y = 0;
  CalcScrolledPosition(0, unscrolled_y, NULL, &y);
  return y;
}

}  // namespace ui
}  // namespace jil
