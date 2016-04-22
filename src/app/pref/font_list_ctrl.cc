#include "app/pref/font_list_ctrl.h"
#include "wx/dcbuffer.h"
#include "wx/listctrl.h"
#include "wx/settings.h"

namespace jil {
namespace pref {

BEGIN_EVENT_TABLE(FontListCtrl, wxScrolledWindow)
EVT_SIZE(FontListCtrl::OnSize)
EVT_PAINT(FontListCtrl::OnPaint)
EVT_LEFT_DOWN(FontListCtrl::OnMouseLeftDown)
EVT_SET_FOCUS(FontListCtrl::OnSetFocus)
EVT_KILL_FOCUS(FontListCtrl::OnKillFocus)
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

  SetWindowStyleFlag(wxBORDER_STATIC);

  int cw = GetCharWidth();
  int ch = GetCharHeight();
  padding_.Set(cw, cw/2+1);

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
  int h = 0;
  wxClientDC dc(this);
   
  for (size_t i = 0; i < rows_.size(); ++i) {
    Row& row = rows_[i];

    dc.SetFont(row.font);
    wxSize label_size = dc.GetTextExtent(row.label);
    row.h = label_size.y + padding_.y * 2;

    row.y = h;

    h += row.h;
  }

  SetVirtualSize(wxSize(-1, h));
  AdjustScrollbars();
}

void FontListCtrl::InitColors() {
  SetColor(COLOR_BG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
  SetColor(COLOR_BG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
  SetColor(COLOR_BG_HL_NOFOCUS, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
  SetColor(COLOR_FG, wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
  SetColor(COLOR_FG_HL, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
  SetColor(COLOR_BORDER, wxColour(225, 215, 225));  // TODO: wxSYS_COLOUR_ACTIVEBORDER
}

void FontListCtrl::OnSize(wxSizeEvent& evt) {
  UpdateSizes();
  Refresh();
}

void FontListCtrl::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
  dc.SetBackground(wxBrush(GetBackgroundColour()));
  dc.Clear();

  PrepareDC(dc);

  wxColour bg_hl_color = GetColor(COLOR_BG_HL);
  wxColour bg_hl_nofocus_color = GetColor(COLOR_BG_HL_NOFOCUS);
  wxColour fg_color = GetColor(COLOR_FG);
  wxColour fg_hl_color = GetColor(COLOR_FG_HL);
  wxColour border_color = GetColor(COLOR_BORDER);

  dc.SetTextForeground(fg_color);
  dc.SetPen(border_color);

  wxRect rect = GetClientRect();

  int x = rect.x;
  int y = rect.y;

  for (size_t i = 0; i < rows_.size(); ++i) {
    const Row& row = rows_[i];

    dc.SetFont(row.font);

    int row_y = y + padding_.y;

    if (i == selected_index_) {
      dc.SetTextForeground(fg_hl_color);
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(HasFocus() ? bg_hl_color : bg_hl_nofocus_color);

      wxRect row_rect(rect.x + 1, y, rect.width - 1, row.h);
      dc.DrawRectangle(row_rect);

      dc.DrawText(row.label, x + padding_.x, row_y);

      // Restore
      dc.SetTextForeground(fg_color);
      dc.SetPen(border_color);
    } else {
      dc.DrawText(row.label, x + padding_.x, row_y);
    }

    y += row.h;

    // Border
    dc.DrawLine(rect.x, y, rect.GetRight(), y);
  }
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

void FontListCtrl::OnSetFocus(wxFocusEvent& evt) {
  evt.Skip();

  if (selected_index_ != wxNOT_FOUND) {
    Refresh();
  }
}

void FontListCtrl::OnKillFocus(wxFocusEvent& evt) {
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
