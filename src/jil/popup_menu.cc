#include "jil/popup_menu.h"

#include "wx/dcbuffer.h"
#include "wx/dcclient.h"
#include "wx/log.h"
#include "wx/sizer.h"

#include "ui/util.h"

#include "editor/defs.h"
#include "editor/text_extent.h"

namespace jil {

using editor::kNpos;

static const int BORDER = 1;  // 1px border size.

////////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(PopupMenu, wxPopupTransientWindow)
EVT_PAINT(PopupMenu::OnPaint)
EVT_LEFT_DOWN(PopupMenu::OnMouseLeftDown)
EVT_LEFT_UP(PopupMenu::OnMouseLeftUp)
EVT_MOTION(PopupMenu::OnMouseMotion)
END_EVENT_TABLE()

PopupMenu::PopupMenu(const editor::SharedTheme& theme)
    : theme_(theme)
    , margin_(2, 2)
    , item_padding_(5, 3)
    , col_gap_(10)
    , row_height_(0)
    , select_index_(kNpos)
    , column_width_(180)  // TODO
    , max_rows_(0)
    , rows_(0)
    , cols_(0) {
}

PopupMenu::~PopupMenu() {
}

bool PopupMenu::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPopupTransientWindow::Create(parent, wxBORDER_NONE)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);

  if (theme_->GetColor(COLOR_BG).IsOk()) {
    SetBackgroundColour(theme_->GetColor(COLOR_BG));
  }

  row_height_ = GetCharHeight() + item_padding_.y + item_padding_.y;

  return true;
}

void PopupMenu::Dismiss() {
  wxPopupTransientWindow::Dismiss();
  Destroy();
}

void PopupMenu::Popup(wxWindow* focus) {
  AdjustSizeAndPosition();

  wxPopupTransientWindow::Popup(focus);
}

void PopupMenu::SetTabs(const std::vector<wxString>& tabs) {
  items_ = tabs;
  item_rects_.resize(tabs.size());
}

void PopupMenu::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);

  wxRect client_rect = GetClientRect();

  // Border
  dc.SetPen(wxPen(theme_->GetColor(COLOR_BORDER)));
  dc.SetBrush(wxBrush(theme_->GetColor(COLOR_BG)));
  dc.DrawRectangle(client_rect);

  wxRect update_rect = GetUpdateClientRect();

  dc.SetFont(GetFont());

  // Draw hover item firstly.
  if (select_index_ != kNpos) {
    wxRect item_rect = item_rects_[select_index_];

    if (update_rect.Intersects(item_rects_[select_index_])) {
      dc.SetPen(wxPen(theme_->GetColor(COLOR_SELECT_BORDER)));
      dc.SetBrush(wxBrush(theme_->GetColor(COLOR_SELECT_BG)));
      dc.SetTextForeground(theme_->GetColor(COLOR_SELECT_FG));

      dc.DrawRectangle(item_rect);

      item_rect.Deflate(item_padding_.x, item_padding_.y);
      ui::DrawTextInRect(dc, items_[select_index_], item_rect);
    }
  }

  dc.SetTextForeground(theme_->GetColor(COLOR_FG));

  for (size_t i = 0; i < items_.size(); ++i) {
    if (i != select_index_) {
      wxRect item_rect = item_rects_[i];
      if (update_rect.Intersects(item_rect)) {
        item_rect.Deflate(item_padding_.x, item_padding_.y);
        ui::DrawTextInRect(dc, items_[i], item_rect);
      }
    }
  }
}

void PopupMenu::OnMouseLeftDown(wxMouseEvent& evt) {
  if (!HasCapture()) {
    CaptureMouse();
  }
}

void PopupMenu::OnMouseLeftUp(wxMouseEvent& evt) {
  if (!HasCapture()) {
    return;
  }
  ReleaseMouse();

  size_t index = GetIndexByPos(evt.GetPosition());

  if (index != kNpos) {
    // Post an event to parent window.
    if (GetParent() != NULL) {
      PopupMenuEvent pm_evt(kEvtPopupMenuSelect);
      pm_evt.set_index(index);
      pm_evt.set_label(items_[index]);

      GetParent()->GetEventHandler()->AddPendingEvent(pm_evt);
    }

    // NOTE:
    Dismiss();
  } else {
    evt.Skip();
  }
}

void PopupMenu::OnMouseMotion(wxMouseEvent& evt) {
  size_t index = GetIndexByPos(evt.GetPosition());

  if (select_index_ == index) {
    return;
  }

  select_index_ = index;
  Refresh();  // TODO: Optimize
}

void PopupMenu::AdjustSizeAndPosition() {
  int up_space = owner_rect_.GetTop() - display_client_rect_.GetTop();
  int down_space = display_client_rect_.GetBottom() - owner_rect_.GetBottom();

  bool down = (down_space >= up_space);

  if (down) {
    CalcMaxRows(down_space);
  } else {
    CalcMaxRows(up_space);
  }

  CalcRowsCols();

  wxSize client_size;

  client_size.x = column_width_ * cols_ + col_gap_ * (cols_ - 1);
  client_size.x += margin_.x + margin_.x + BORDER + BORDER;

  client_size.y = row_height_ * rows_ + col_gap_ * (cols_ - 1);
  client_size.y += margin_.y + margin_.y + BORDER + BORDER;

  SetClientSize(client_size);

  UpdateItemRects();

  // Get best position.
  // NOTE:
  // Don't rely on wxPopupTransientWindow::Position() since we need to
  // display multiple "columns" if necessary.

  wxPoint pos;

  pos.x = owner_rect_.GetLeft();
  if (pos.x + client_size.x > display_client_rect_.GetRight()) {
    pos.x = display_client_rect_.GetRight() - client_size.x;
  }

  if (down) {
    pos.y = owner_rect_.GetBottom();
  } else {
    pos.y = owner_rect_.GetTop() - client_size.y;
  }

  SetPosition(pos);
}

void PopupMenu::UpdateItemRects() {
  for (size_t i = 0; i < items_.size(); ++i) {
    int col = i / rows_;
    int row = i % rows_;

    int x = BORDER + margin_.x + (column_width_ + col_gap_) * col;
    int y = BORDER + margin_.y + row_height_ * row;

    item_rects_[i] = wxRect(x, y, column_width_, row_height_);
  }
}

void PopupMenu::CalcMaxRows(int total_height) {
  max_rows_ = (total_height - margin_.y - margin_.y - BORDER - BORDER) / row_height_;
}

void PopupMenu::CalcRowsCols() {
  int rows = static_cast<int>(items_.size());

  if (rows < max_rows_) {
    cols_ = 1;
    rows_ = rows;
  } else {
    cols_ = rows / max_rows_;

    if ((rows % max_rows_) > 0) {
      ++cols_;
    }

    rows_ = max_rows_;
  }
}

size_t PopupMenu::GetIndexByPos(const wxPoint& pos) const {
  for (size_t i = 0; i < item_rects_.size(); ++i) {
    if (item_rects_[i].Contains(pos)) {
      return i;
    }
  }
  return kNpos;
}

////////////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(kEvtPopupMenuSelect);
IMPLEMENT_DYNAMIC_CLASS(PopupMenuEvent, wxEvent)

PopupMenuEvent::PopupMenuEvent(wxEventType type)
    : wxEvent(0, type), index_(kNpos) {
}

PopupMenuEvent::PopupMenuEvent(const PopupMenuEvent& rhs)
    : wxEvent(rhs)
    , index_(rhs.index_)
    , label_(rhs.label_) {
}

}  // namespace jil
