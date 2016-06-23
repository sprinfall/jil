#include "app/pref/font_list_ctrl.h"
#include "wx/dcbuffer.h"
#include "wx/listctrl.h"
#include "wx/settings.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(FontListCtrl, wxScrolledWindow)
EVT_SIZE        (FontListCtrl::OnSize)
EVT_PAINT       (FontListCtrl::OnPaint)
EVT_LEFT_DOWN   (FontListCtrl::OnMouseLeftDown)
EVT_SET_FOCUS   (FontListCtrl::OnFocusChange)
EVT_KILL_FOCUS  (FontListCtrl::OnFocusChange)
END_EVENT_TABLE()

FontListCtrl::FontListCtrl()
    : selected_index_(wxNOT_FOUND) {
  InitColors();
}

FontListCtrl::~FontListCtrl() {
}

bool FontListCtrl::Create(wxWindow* parent, wxWindowID id, const wxSize& size, long style) {
  if (!wxScrolledWindow::Create(parent, id, wxDefaultPosition, size, style)) {
    return false;
  }

  SetBackgroundColour(GetColor(COLOR_BG));
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  SetWindowStyleFlag(wxBORDER_SIMPLE);

  int cw = GetCharWidth();
  int ch = GetCharHeight();
  padding_.Set(cw, cw / 2 + 1);

  SetScrollbars(cw, ch, 1, 1);

  return true;
}

void FontListCtrl::AddFont(const wxFont& font, const wxString& label) {
  assert(font.IsOk());

  Row row = { font, label };
  rows_.push_back(row);
}

void FontListCtrl::SetFont(int index, const wxFont& font) {
  assert(index >= 0 && index < GetCount());

  rows_[index].font = font;

  UpdateSizes();
  Refresh();
}

void FontListCtrl::UpdateSizes() {
  int w = 0;
  int h = 0;
  wxClientDC dc(this);
   
  for (size_t i = 0; i < rows_.size(); ++i) {
    Row& row = rows_[i];

    dc.SetFont(row.font);
    wxSize label_size = dc.GetTextExtent(row.label);

    row.h = label_size.y + padding_.y * 2;
    row.y = h;

    int row_width = label_size.x + padding_.x * 2;
    if (w < row_width) {
      w = row_width;
    }

    h += row.h;
  }

  wxSize best_size(w, h);
  //SetInitialSize(best_size);

  SetVirtualSize(best_size);
  AdjustScrollbars();
}

void FontListCtrl::InitColors() {
  SetColor(COLOR_BG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
  SetColor(COLOR_BG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
  SetColor(COLOR_BG_HL_NOFOCUS, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
  SetColor(COLOR_FG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
  SetColor(COLOR_FG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
  SetColor(COLOR_BORDER, wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVEBORDER));
}

//wxSize FontListCtrl::DoGetBestSize() const {
//  if (rows_.empty()) {
//    return wxDefaultSize;
//  }
//
//  int best_height = 0;
//  for (size_t i = 0; i < rows_.size(); ++i) {
//    best_height += rows_[i].h;
//  }
//  return wxSize(best_height, max_row_width_);
//}

void FontListCtrl::OnSize(wxSizeEvent& evt) {
  UpdateSizes();
  Refresh();
}

void FontListCtrl::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();

  PrepareDC(dc);

  dc.SetTextForeground(GetColor(COLOR_FG));
  dc.SetPen(GetColor(COLOR_BORDER));

  wxRect rect = GetClientRect();
  int y = rect.y;

  for (size_t i = 0; i < rows_.size(); ++i) {
    const Row& row = rows_[i];

    dc.SetFont(row.font);

    if (i == selected_index_) {
      wxRect row_rect = wxRect(rect.x, y, rect.width, row.h);
      DrawSelectedRow(dc, row_rect, row.label);
    } else {
      dc.DrawText(row.label, rect.x + padding_.x, y + padding_.y);
    }

    y += row.h;

    // Border
    int border_y = y - 1;
    dc.DrawLine(rect.x, border_y, rect.GetRight(), border_y);
  }
}

void FontListCtrl::DrawSelectedRow(wxDC& dc, const wxRect& row_rect, const wxString& label) {
  // Backup
  wxPen pen = dc.GetPen();

  wxColour hl_color = GetColor(HasFocus() ? COLOR_BG_HL : COLOR_BG_HL_NOFOCUS);
  dc.SetPen(hl_color);
  dc.SetBrush(hl_color);
  dc.DrawRectangle(row_rect);

  dc.SetTextForeground(GetColor(COLOR_FG_HL));
  dc.DrawText(label, row_rect.x + padding_.x, row_rect.y + padding_.y);

  // Restore
  dc.SetTextForeground(GetColor(COLOR_FG));
  dc.SetPen(pen);
}

void FontListCtrl::OnMouseLeftDown(wxMouseEvent& evt) {
  evt.Skip();  // For focus.

  wxPoint pos = evt.GetPosition();

  int unscrolled_x = 0;
  int unscrolled_y = 0;
  CalcUnscrolledPosition(pos.x, pos.y, &unscrolled_x, &unscrolled_y);

  int y = 0;
  int new_selected_index = wxNOT_FOUND;
  for (size_t i = 0; i < rows_.size(); ++i) {
    Row& row = rows_[i];

    if (unscrolled_y > y && unscrolled_y <= y + row.h) {
      new_selected_index = static_cast<int>(i);
    }

    y += row.h;
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

void FontListCtrl::OnFocusChange(wxFocusEvent& evt) {
  evt.Skip();

  if (selected_index_ != wxNOT_FOUND) {
    Refresh();
  }
}

void FontListCtrl::PostEvent(int index, bool select) {
  wxEventType evt_type = select ? wxEVT_LIST_ITEM_SELECTED : wxEVT_LIST_ITEM_DESELECTED;
  wxListEvent evt(evt_type, GetId());
  evt.SetEventObject(this);
  evt.m_itemIndex = index;
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace pref
}  // namespace jil
