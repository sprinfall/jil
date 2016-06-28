#include "ui/string_list_ctrl.h"
#include "wx/dcbuffer.h"
#include "wx/listctrl.h"
#include "wx/settings.h"

namespace jil {
namespace ui {

BEGIN_EVENT_TABLE(StringListCtrl, wxScrolledWindow)
EVT_PAINT       (StringListCtrl::OnPaint)
EVT_LEFT_DOWN   (StringListCtrl::OnMouseLeftDown)
EVT_SET_FOCUS   (StringListCtrl::OnFocusChange)
EVT_KILL_FOCUS  (StringListCtrl::OnFocusChange)
END_EVENT_TABLE()

StringListCtrl::StringListCtrl()
    : selected_index_(wxNOT_FOUND) {
  InitColors();
}

StringListCtrl::~StringListCtrl() {
}

bool StringListCtrl::Create(wxWindow* parent, wxWindowID id, const wxSize& size, long style) {
  if (!wxScrolledWindow::Create(parent, id, wxDefaultPosition, size, style)) {
    return false;
  }

  SetBackgroundColour(GetColor(COLOR_BG));
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  SetWindowStyleFlag(wxBORDER_STATIC);

  UpdateSizes();

  return true;
}

void StringListCtrl::AppendString(const wxString& string) {
  strings_.push_back(string);
}

void StringListCtrl::UpdateSizes() {
  InvalidateBestSize();

  int cw = GetCharWidth();
  int ch = GetCharHeight();

  raw_padding_.Set(cw, cw / 2 + 1);

  row_size_.x = 0;
  row_size_.y = ch + raw_padding_.y * 2;

  for (size_t i = 0; i < strings_.size(); ++i) {
    int label_w = 0;
    GetTextExtent(strings_[i], &label_w, NULL);

    if (row_size_.x < label_w) {
      row_size_.x = label_w;
    }
  }

  row_size_.x += raw_padding_.x * 2;

  wxSize vsize(row_size_.x, row_size_.y * (strings_.size() + 1));
  SetVirtualSize(vsize);

  SetScrollbars(cw, row_size_.y, 1, 1);
  AdjustScrollbars();
}

void StringListCtrl::InitColors() {
  SetColor(COLOR_BG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
  SetColor(COLOR_BG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
  SetColor(COLOR_FG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
  SetColor(COLOR_FG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
  //SetColor(COLOR_BORDER, wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVEBORDER));
}

wxSize StringListCtrl::DoGetBestSize() const {
  if (strings_.empty()) {
    return wxDefaultSize;
  }
  return wxSize(row_size_.x, row_size_.y * (strings_.size() + 1));
}

void StringListCtrl::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  PrepareDC(dc);

  dc.SetTextForeground(GetColor(COLOR_FG));
  //dc.SetPen(GetColor(COLOR_BORDER));
  dc.SetFont(GetFont());

  wxRect rect = GetClientRect();
  int y = rect.y;

  for (size_t i = 0; i < strings_.size(); ++i) {
    if (i == selected_index_) {
      wxRect row_rect = wxRect(rect.x, y, rect.width, row_size_.y);
      DrawSelectedRow(dc, row_rect, strings_[i]);
    } else {
      dc.DrawText(strings_[i], rect.x + raw_padding_.x, y + raw_padding_.y);
    }

    y += row_size_.y;
  }
}

void StringListCtrl::DrawSelectedRow(wxDC& dc, const wxRect& row_rect, const wxString& label) {
  // Backup
  wxPen pen = dc.GetPen();

  wxColour hl_color = GetColor(COLOR_BG_HL);
  dc.SetPen(hl_color);
  dc.SetBrush(hl_color);
  dc.DrawRectangle(row_rect);

  dc.SetTextForeground(GetColor(COLOR_FG_HL));
  dc.DrawText(label, row_rect.x + raw_padding_.x, row_rect.y + raw_padding_.y);

  // Restore
  dc.SetTextForeground(GetColor(COLOR_FG));
  dc.SetPen(pen);
}

void StringListCtrl::OnMouseLeftDown(wxMouseEvent& evt) {
  evt.Skip();  // For focus.

  wxPoint pos = evt.GetPosition();

  int unscrolled_y = 0;
  CalcUnscrolledPosition(0, pos.y, NULL, &unscrolled_y);

  int new_selected_index = unscrolled_y / row_size_.y;
  if (new_selected_index >= static_cast<int>(strings_.size())) {
    new_selected_index = wxNOT_FOUND;
  }

  int old_selected_index = selected_index_;
  selected_index_ = new_selected_index;

  if (selected_index_ != old_selected_index) {
    Refresh();  // TODO

    if (old_selected_index != wxNOT_FOUND) {
      PostEvent(old_selected_index, false);
      //RefreshRow(old_selected_index);
    }

    if (selected_index_ != wxNOT_FOUND) {
      PostEvent(selected_index_, true);
      //RefreshRow(selected_index_);
    }
  }
}

void StringListCtrl::OnFocusChange(wxFocusEvent& evt) {
  evt.Skip();

  if (selected_index_ != wxNOT_FOUND) {
    Refresh();
  }
}

void StringListCtrl::PostEvent(int index, bool select) {
  wxEventType evt_type = select ? wxEVT_LIST_ITEM_SELECTED : wxEVT_LIST_ITEM_DESELECTED;
  wxListEvent evt(evt_type, GetId());
  evt.SetEventObject(this);
  evt.m_itemIndex = index;
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace ui
}  // namespace jil
