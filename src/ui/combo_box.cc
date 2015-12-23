#include "ui/combo_box.h"
#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include "wx/graphics.h"

namespace jil {
namespace ui {

////////////////////////////////////////////////////////////////////////////////

void ComboStyle::Fix() {
  for (int i = 0; i < PARTS; ++i) {
    for (int j = 1; j < STATES; ++j) {
      if (!colors_[i][j].IsOk()) {
        colors_[i][j] = colors_[i][j - 1];
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static const size_t kNpos = static_cast<size_t>(-1);

static const long kPopupStyle = wxFRAME_SHAPED | wxNO_BORDER | wxSTAY_ON_TOP;

BEGIN_EVENT_TABLE(ComboPopup, wxDialog)
EVT_PAINT           (ComboPopup::OnPaint)
EVT_ACTIVATE        (ComboPopup::OnActivate)
EVT_MOTION          (ComboPopup::OnMouseMotion)
EVT_LEFT_UP         (ComboPopup::OnMouseLeftUp)
EVT_LEAVE_WINDOW    (ComboPopup::OnMouseLeave)
EVT_CHAR_HOOK       (ComboPopup::OnKeyDownHook)
END_EVENT_TABLE()

ComboPopup::ComboPopup()
    : hover_index_(kNpos)
    , margin_(5, 5)
    , padding_(5, 2)
    , space_y_(4)
    , separator_height_(4) {
}

bool ComboPopup::Create(wxWindow* parent, wxWindowID id) {
  if (!wxDialog::Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, kPopupStyle)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);

  if (colors_[BG].IsOk()) {
    SetBackgroundColour(colors_[BG]);
  }

  return true;
}

ComboPopup::~ComboPopup() {
}

void ComboPopup::Append(const wxString& label) {
  labels_.push_back(label);
}

void ComboPopup::AppendSeparator() {
  labels_.push_back(wxEmptyString);
}

void ComboPopup::SetLabels(const std::vector<wxString>& labels) {
  labels_ = labels;
}

void ComboPopup::AdjustSize() {
  item_rects_.resize(labels_.size());

  int y = margin_.y;

  int label_w = 0;
  int label_h = 0;
  int max_label_w = 0;

  for (size_t i = 0; i < labels_.size(); ++i) {
    if (!labels_[i].IsEmpty()) {
      GetTextExtent(labels_[i], &label_w, &label_h);
      if (max_label_w < label_w) {
        max_label_w = label_w;
      }
    } else {  // Separator
      label_h = separator_height_;
    }

    // Rect width will be set later.
    item_rects_[i] = wxRect(margin_.x, y, 0, label_h + padding_.y + padding_.y);
    y += item_rects_[i].height + space_y_;
  }

  y += margin_.y - space_y_;

  // Now set the width of item rects.
  int item_w = max_label_w + padding_.x + padding_.x;
  for (size_t i = 0; i < item_rects_.size(); ++i) {
    item_rects_[i].width = item_w;
  }

  int client_w = item_w + margin_.x + margin_.x;
  SetClientSize(client_w, y);
}

void ComboPopup::OnPaint(wxPaintEvent& evt) {
  if (item_rects_.empty()) {
    evt.Skip();
    return;
  }

  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
#endif

  // Always LTR
  // TODO: Apply to other user-defined controls.
  dc.SetLayoutDirection(wxLayout_LeftToRight);

  dc.SetPen(wxPen(colors_[BORDER]));
  dc.SetBrush(*wxTRANSPARENT_BRUSH);

  wxRect rect = GetClientRect();
  dc.DrawRectangle(rect);

  wxRect update_rect = GetUpdateClientRect();

  if (hover_index_ != kNpos) {
    wxRect& rect = item_rects_[hover_index_];
    if (rect.Intersects(update_rect)) {
      dc.SetPen(wxPen(colors_[BG_HOVER]));
      dc.SetBrush(wxBrush(colors_[BG_HOVER]));
      dc.DrawRectangle(rect);
    }
  }

  dc.SetFont(GetFont());

  for (size_t i = 0; i < labels_.size(); ++i) {
    wxRect& rect = item_rects_[i];

    if (!rect.Intersects(update_rect)) {
      continue;
    }

    const wxString& label = labels_[i];

    if (label.IsEmpty()) {  // Separator
      dc.SetPen(wxPen(colors_[SEPARATOR]));
      int y = rect.y + rect.height / 2;
      dc.DrawLine(padding_.x, y, rect.GetRight(), y);
    } else {
      dc.SetTextForeground(colors_[FG]);
      dc.DrawText(label, rect.x + padding_.x, rect.y + padding_.y);
    }
  }
}

void ComboPopup::OnActivate(wxActivateEvent& evt) {
  if (!evt.GetActive()) {
    wxCommandEvent combo_evt(wxEVT_COMBOBOX_CLOSEUP, GetId());
    combo_evt.SetEventObject(this);
    GetParent()->GetEventHandler()->AddPendingEvent(combo_evt);
  } else {
    evt.Skip();
  }
}

void ComboPopup::OnMouseMotion(wxMouseEvent& evt) {
  size_t index = IndexItem(evt.GetPosition());

  if (hover_index_ == index) {
    return;
  }

  size_t old_hover_index = hover_index_;
  hover_index_ = index;

  RefreshByIndex(old_hover_index);

  if (hover_index_ != kNpos) {
    RefreshByIndex(hover_index_);
  }
}

void ComboPopup::OnMouseLeftUp(wxMouseEvent& evt) {
  size_t index = IndexItem(evt.GetPosition());
  if (index == kNpos) {
    return;
  }

  wxCommandEvent combo_evt(wxEVT_COMMAND_COMBOBOX_SELECTED, GetId());
  combo_evt.SetEventObject(this);
  combo_evt.SetInt(index);  // Use GetInt() or GetSelection() to get it.
  combo_evt.SetString(labels_[index]);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

void ComboPopup::OnMouseLeave(wxMouseEvent& evt) {
  if (hover_index_ != kNpos) {
    size_t index = hover_index_;
    hover_index_ = kNpos;
    RefreshByIndex(index);
  }
}

void ComboPopup::OnKeyDownHook(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_ESCAPE) {
    Destroy();
  } else {
    evt.Skip();
  }
}

size_t ComboPopup::IndexItem(const wxPoint& pos) {
  for (size_t i = 0; i < item_rects_.size(); ++i) {
    if (pos.y >= item_rects_[i].y && pos.y < item_rects_[i].GetBottom()) {
      if (!labels_[i].IsEmpty()) {
        return i;
      } else {
        return kNpos;
      }
    }
  }
  return kNpos;
}

void ComboPopup::RefreshByIndex(size_t index) {
  if (index != kNpos) {
    Refresh(true, &item_rects_[index]);
  }
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ComboBox, wxControl)
EVT_PAINT               (ComboBox::OnPaint)
EVT_LEFT_DOWN           (ComboBox::OnMouseLeftDown)
EVT_LEFT_UP             (ComboBox::OnMouseLeftUp)
EVT_ENTER_WINDOW        (ComboBox::OnMouseEnter)
EVT_LEAVE_WINDOW        (ComboBox::OnMouseLeave)
EVT_MOUSE_CAPTURE_LOST  (ComboBox::OnMouseCaptureLost)
EVT_KEY_UP              (ComboBox::OnKeyUp)
EVT_SET_FOCUS           (ComboBox::OnSetFocus)
EVT_KILL_FOCUS          (ComboBox::OnKillFocus)
END_EVENT_TABLE()

ComboBox::ComboBox(SharedComboStyle style)
    : style_(style)
    , pressed_(false)
    , hover_(false)
    , accept_focus_(true)
    , down_(true)
    , popup_(NULL) {
}

bool ComboBox::Create(wxWindow* parent, wxWindowID id, const wxString& label) {
  if (!wxControl::Create(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  SetBackgroundColour(parent->GetBackgroundColour());

  SetLayoutDirection(wxLayout_LeftToRight);

  int cw = GetCharWidth();
  padding_.Set(cw, cw / 2 + 1);

  SetLabel(label);

  return true;
}

ComboBox::~ComboBox() {
}

void ComboBox::Append(const wxString& label) {
  labels_.push_back(label);
}

wxSize ComboBox::DoGetBestSize() const {
  wxSize best_size;
  GetTextExtent(GetLabel(), &best_size.x, &best_size.y);
  best_size.IncBy(padding_.x * 2, padding_.y * 2);
  return best_size;
}

ComboStyle::State ComboBox::GetState() const {
  if (pressed_) {
    return hover_ ? ComboStyle::PRESSED_HOVER : ComboStyle::PRESSED;
  } else {
    return hover_ ? ComboStyle::NORMAL_HOVER : ComboStyle::NORMAL;
  }
}

void ComboBox::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();
#endif

#ifdef __WXMAC__
  wxGraphicsContext* gc = dc.GetGraphicsContext();
#else
  wxGCDC gcdc(dc);
  wxGraphicsContext* gc = gcdc.GetGraphicsContext();
#endif // __WXMAC__

  ComboStyle::State state = GetState();

  wxColour fg = style_->GetColor(ComboStyle::FG, state);
  wxColour bg = style_->GetColor(ComboStyle::BG, state);
  wxColour border = style_->GetColor(ComboStyle::BORDER, state);

  wxRect client_rect = GetClientRect();

  dc.SetPen(wxPen(border));
  //dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.SetBrush(wxBrush(bg));
  dc.DrawRectangle(client_rect);

  dc.SetTextForeground(fg);
  dc.SetTextBackground(wxNullColour);
  dc.SetFont(GetFont());

  dc.DrawText(GetLabel(), padding_.x, padding_.y);

  DrawArrow(gc, state);
}

void ComboBox::DrawArrow(wxGraphicsContext* gc, ComboStyle::State state) {
  wxColour border = style_->GetColor(ComboStyle::ARROW_BORDER, state);
  wxColour fill = style_->GetColor(ComboStyle::ARROW_FILL, state);

  gc->SetPen(wxPen(border));
  gc->SetBrush(wxBrush(fill));

  wxRect rect = GetClientRect();

  int w = GetCharWidth();
  int h = w / 2 + 1;
  int x = rect.GetRight() - 10;
  int y = rect.y + (rect.height - h) / 2;

  wxGraphicsPath path = gc->CreatePath();

  if (down_) {
    path.MoveToPoint(x, y + h);
    path.AddLineToPoint(x - w / 2, y);
    path.AddLineToPoint(x + w / 2, y);
  } else {
    path.MoveToPoint(x, y);
    path.AddLineToPoint(x - w / 2, y + h);
    path.AddLineToPoint(x + w / 2, y + h);
  }

  path.CloseSubpath();
  gc->DrawPath(path);
}

void ComboBox::OnMouseLeftDown(wxMouseEvent& evt) {
  pressed_ = true;

  Refresh();

  if (!HasCapture()) {
    CaptureMouse();
  }

  if (popup_ != NULL) {
    popup_->Destroy();
    popup_ = NULL;
    evt.Skip();
  } else {
    ShowPopup();
  }
}

void ComboBox::OnMouseLeftUp(wxMouseEvent& evt) {
  if (HasCapture()) {
    ReleaseMouse();
  }

  pressed_ = false;
  Refresh();

  evt.Skip();
}

void ComboBox::OnMouseEnter(wxMouseEvent& evt) {
  hover_ = true;
  Refresh();
  evt.Skip();
}

void ComboBox::OnMouseLeave(wxMouseEvent& evt) {
  hover_ = false;
  Refresh();
  evt.Skip();
}

void ComboBox::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  pressed_ = false;
  hover_ = false;
  Refresh();
}

void ComboBox::OnKeyUp(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_SPACE) {
    // TODO: Popup
  }
  evt.Skip();
}

void ComboBox::OnSetFocus(wxFocusEvent& evt) {
  Refresh();
  evt.Skip();
}

void ComboBox::OnKillFocus(wxFocusEvent& evt) {
  Refresh();
  evt.Skip();
}

void ComboBox::OnPopupClose(wxCommandEvent& evt) {
  ClosePopup();
}

void ComboBox::OnPopupSelected(wxCommandEvent& evt) {
  ClosePopup();

  wxCommandEvent combo_evt(wxEVT_COMMAND_COMBOBOX_SELECTED, GetId());
  combo_evt.SetEventObject(this);
  combo_evt.SetInt(evt.GetSelection());
  combo_evt.SetString(evt.GetString());
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

void ComboBox::ShowPopup() {
  popup_ = new ComboPopup();

  popup_->SetColor(ComboPopup::BG, style_->GetColor(ComboStyle::BG, ComboStyle::NORMAL));
  popup_->SetColor(ComboPopup::FG, style_->GetColor(ComboStyle::FG, ComboStyle::NORMAL));
  popup_->SetColor(ComboPopup::BG_HOVER, style_->GetColor(ComboStyle::BG, ComboStyle::NORMAL_HOVER));
  popup_->SetColor(ComboPopup::FG_HOVER, style_->GetColor(ComboStyle::FG, ComboStyle::NORMAL_HOVER));
  popup_->SetColor(ComboPopup::BORDER, style_->GetColor(ComboStyle::BORDER, ComboStyle::NORMAL));
  popup_->SetColor(ComboPopup::SEPARATOR, style_->GetColor(ComboStyle::BORDER, ComboStyle::NORMAL));

  popup_->Create(this, wxID_ANY);
  popup_->SetLabels(labels_);

  ConnectPopupEventHandlers();

  popup_->AdjustSize();
   
  wxRect rect = GetScreenRect();
  wxPoint pos = rect.GetLeftTop();
  pos.y -= popup_->GetSize().y;
  popup_->SetPosition(pos);

  popup_->Show();
}

void ComboBox::ClosePopup() {
  if (popup_ != NULL) {
    DisconnectPopupEventHandlers();

    popup_->Destroy();
    popup_ = NULL;
  }
}

void ComboBox::ConnectPopupEventHandlers() {
  Connect(popup_->GetId(),
          wxEVT_COMBOBOX_CLOSEUP,
          wxCommandEventHandler(ComboBox::OnPopupClose));

  Connect(popup_->GetId(),
          wxEVT_COMMAND_COMBOBOX_SELECTED,
          wxCommandEventHandler(ComboBox::OnPopupSelected));
}

void ComboBox::DisconnectPopupEventHandlers() {
  Disconnect(popup_->GetId());
}

}  // namespace ui
}  // namespace jil
