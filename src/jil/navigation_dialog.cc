#include "jil/navigation_dialog.h"
#include "wx/button.h"
#include "wx/dcbuffer.h"
#include "wx/dcclient.h"
#include "wx/log.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "ui/util.h"
#include "editor/defs.h"
#include "editor/text_extent.h"
#include "jil/text_page.h"

#define kTrTextPages _("Text Pages")

namespace jil {

static const int kSpaceY = 7;
static const int kPaddingX = 5;
static const int kPaddingY = 3;

using editor::kNpos;

BEGIN_EVENT_TABLE(NavigationDialog, wxDialog)
EVT_PAINT         (NavigationDialog::OnPaint)
EVT_CLOSE         (NavigationDialog::OnClose)
EVT_ACTIVATE      (NavigationDialog::OnActivate)
EVT_LEFT_UP       (NavigationDialog::OnMouseLeftUp)
EVT_MOTION        (NavigationDialog::OnMouseMotion)
// EVT_CHAR_HOOK     (NavigationDialog::OnKeyDown)
EVT_KEY_DOWN      (NavigationDialog::OnKeyDown)
EVT_KEY_UP        (NavigationDialog::OnKeyUp)
EVT_NAVIGATION_KEY(NavigationDialog::OnNavigationKey)
END_EVENT_TABLE()

NavigationDialog::NavigationDialog(const editor::SharedTheme& theme)
    : theme_(theme)
    , margin_(10, 10)
    , col_gap_(10)
    , select_index_(kNpos)
    , column_width_(180)
    , max_rows_(10) {
}

NavigationDialog::~NavigationDialog() {
}

bool NavigationDialog::Create(wxWindow* parent, wxWindowID id) {
  long style = wxWANTS_CHARS;
  if (!wxDialog::Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, style)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);

  if (theme_->GetColor(COLOR_BG).IsOk()) {
    SetBackgroundColour(theme_->GetColor(COLOR_BG));
  }

  title_font_ = GetFont().Bold();

  return true;
}

int NavigationDialog::ShowModal() {
  AdjustSize();
  CenterOnParent();
  return wxDialog::ShowModal();
}

void NavigationDialog::SetTextPages(const std::vector<TextPage*>& text_pages, bool forward) {
  assert(text_pages.size() > 1);

  text_pages_ = text_pages;
  text_rects_.resize(text_pages.size());

  if (forward) {
    select_index_ = 1;  // The second
  } else {
    select_index_ = text_pages.size() - 1;  // The last
  }
}

bool NavigationDialog::DoNavigateIn(int flags) {
  wxLogDebug("do navigate in");
  return wxDialog::DoNavigateIn(flags);
}

void NavigationDialog::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);

  wxRect client_rect = GetClientRect();

  // Border
  dc.SetPen(wxPen(theme_->GetColor(COLOR_BORDER)));
  dc.SetBrush(wxBrush(theme_->GetColor(COLOR_BG)));
  dc.DrawRectangle(client_rect);

  if (select_index_ < text_pages_.size()) {
    dc.SetFont(title_font_);

    ui::DrawTextInRect(dc, kTrTextPages, title_rect_);

    // Selected file name
    dc.SetFont(GetFont());

    dc.SetPen(wxPen(theme_->GetColor(COLOR_SELECT_BORDER)));
    dc.SetBrush(wxBrush(theme_->GetColor(COLOR_SELECT_BG)));
    dc.SetTextForeground(theme_->GetColor(COLOR_SELECT_FG));

    wxRect text_rect = text_rects_[select_index_];
    dc.DrawRectangle(text_rect);

    text_rect.Deflate(kPaddingX, kPaddingY);
    ui::DrawTextInRect(dc, text_pages_[select_index_]->Page_Label(), text_rect);

    // Selected file path
    dc.SetTextForeground(theme_->GetColor(COLOR_FG));

    wxString path = text_pages_[select_index_]->Page_Description();
    ui::DrawTextInRect(dc, path, path_rect_);
  }

  dc.SetFont(GetFont());
  dc.SetTextForeground(theme_->GetColor(COLOR_FG));

  for (std::size_t i = 0; i < text_pages_.size(); ++i) {
    if (i != select_index_) {
      wxRect text_rect = text_rects_[i];
      text_rect.Deflate(kPaddingX, kPaddingY);
      ui::DrawTextInRect(dc, text_pages_[i]->Page_Label(), text_rect);
    }
  }
}

void NavigationDialog::OnClose(wxCloseEvent& evt) {
  // The default close behavior of a dialog doesn't destroy.
  // Destroy explicitly.
  Destroy();
}

void NavigationDialog::OnActivate(wxActivateEvent& evt) {
  if (!evt.GetActive()) {
    Close();
  }
  evt.Skip();
}

void NavigationDialog::OnMouseLeftUp(wxMouseEvent& evt) {
  std::size_t index = GetIndexByPos(evt.GetPosition());

  if (index != kNpos) {
    select_index_ = index;
    Close();
  } else {
    evt.Skip();
  }
}

void NavigationDialog::OnMouseMotion(wxMouseEvent& evt) {
  std::size_t index = GetIndexByPos(evt.GetPosition());

  if (index != kNpos) {
    wxSetCursor(wxCursor(wxCURSOR_HAND));
  } else {
    wxSetCursor(wxCursor(wxCURSOR_ARROW));
  }
}

void NavigationDialog::OnKeyDown(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_ESCAPE) {
    Close();
  } else {
    evt.Skip();
  }
}

void NavigationDialog::OnKeyUp(wxKeyEvent& evt) {
  if (evt.GetKeyCode() == WXK_CONTROL) {
    Close();
  } else {
    evt.Skip();
  }
}

void NavigationDialog::OnNavigationKey(wxNavigationKeyEvent& evt) {
  wxLogDebug("on navigation key");
  if (evt.IsFromTab()) {
    if (evt.GetDirection()) {  // Forward
      ++select_index_;
      if (select_index_ == text_pages_.size()) {
        select_index_ = 0;
      }
    } else {  // Backward
      if (select_index_ == 0) {
        select_index_ = text_pages_.size() - 1;
      } else {
        --select_index_;
      }
    }

    Refresh();
  }
}

void NavigationDialog::AdjustSize() {
  // Calculate the number of columns.
  int cols = text_pages_.size() / max_rows_;
  if ((text_pages_.size() % max_rows_) > 0) {
    ++cols;
  }

  // Double the column width.
  if (cols == 1) {
    column_width_ = column_width_ << 1;
  }

  wxFont font = GetFont();
  const wxString str = wxT("T");

  wxClientDC dc(this);

  int title_h = 0;
  dc.SetFont(title_font_);
  dc.GetTextExtent(str, NULL, &title_h, NULL, NULL, NULL);
  title_h += kPaddingY + kPaddingY;

  int name_h = 0;
  dc.SetFont(font);
  dc.GetTextExtent(str, NULL, &name_h, NULL, NULL, NULL);
  name_h += kPaddingY + kPaddingY;

  int path_h = name_h;

  // Client width without margin.
  int w = column_width_ * cols + col_gap_ * (cols - 1);

  // Client height without margin.
  int h = title_h + kSpaceY + name_h * max_rows_ + kSpaceY + path_h;

  SetClientSize(w + margin_.x + margin_.x, h + margin_.y + margin_.y);

  title_rect_ = wxRect(margin_.x, margin_.y, w, title_h);
  path_rect_ = wxRect(margin_.x, margin_.y + h - path_h, w, path_h);

  for (std::size_t i = 0; i < text_pages_.size(); ++i) {
    int col = i / max_rows_;
    int row = i % max_rows_;

    int x = margin_.x + (column_width_ + col_gap_) * col;
    int y = margin_.y + title_h + kSpaceY + name_h * row;
    text_rects_[i] = wxRect(x, y, column_width_, name_h);
  }
}

std::size_t NavigationDialog::GetIndexByPos(const wxPoint& pos) const {
  for (std::size_t i = 0; i < text_rects_.size(); ++i) {
    if (text_rects_[i].Contains(pos)) {
      return i;
    }
  }
  return kNpos;
}

}  // namespace jil
