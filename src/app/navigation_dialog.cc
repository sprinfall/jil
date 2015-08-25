#include "app/navigation_dialog.h"
#include "wx/dcbuffer.h"
#include "wx/dcclient.h"
#include "wx/log.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "ui/util.h"
#include "editor/defs.h"
#include "editor/text_extent.h"
#include "app/text_page.h"

namespace jil {

static const int kMarginX = 10;
static const int kMarginY = 10;
static const int kSpaceX = 10;
static const int kSpaceY = 7;
static const int kPaddingX = 5;
static const int kPaddingY = 3;

BEGIN_EVENT_TABLE(NavigationDialog, wxDialog)
EVT_PAINT         (NavigationDialog::OnPaint)
EVT_CLOSE         (NavigationDialog::OnClose)
EVT_ACTIVATE      (NavigationDialog::OnActivate)
EVT_LEFT_UP       (NavigationDialog::OnMouseLeftUp)
EVT_MOTION        (NavigationDialog::OnMouseMotion)
EVT_KEY_DOWN      (NavigationDialog::OnKeyDown)
EVT_KEY_UP        (NavigationDialog::OnKeyUp)
EVT_NAVIGATION_KEY(NavigationDialog::OnNavigationKey)
END_EVENT_TABLE()

NavigationDialog::NavigationDialog(wxWindow* parent,
                                   wxWindowID id,
                                   const editor::SharedTheme& theme)
    : wxDialog(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0)
    , theme_(theme)
    , select_index_(editor::kNpos)
    , column_width_(180)
    , max_rows_(10) {
  SetBackgroundStyle(wxBG_STYLE_CUSTOM);

  if (theme_->GetColor(BG).IsOk()) {
    SetBackgroundColour(theme_->GetColor(BG));
  }

  title_font_ = GetFont();
  title_font_.SetWeight(wxFONTWEIGHT_BOLD);
}

NavigationDialog::~NavigationDialog() {
}

int NavigationDialog::ShowModal() {
  AdjustSize();
  CenterOnParent();
  return wxDialog::ShowModal();
}

void NavigationDialog::SetTextPages(const std::vector<TextPage*>& text_pages,
                                    bool forward) {
  assert(text_pages.size() > 1);

  text_pages_ = text_pages;
  text_rects_.resize(text_pages.size());

  if (forward) {
    select_index_ = 1;  // The second
  } else {
    select_index_ = text_pages.size() - 1;  // The last
  }
}

void NavigationDialog::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  int x = kMarginX;
  int y = kMarginY;

  if (select_index_ < text_pages_.size()) {
    // Title
    dc.SetFont(title_font_);

    wxString title = _("Active Files");
    ui::DrawTextInRect(dc, title, title_rect_);

    // Selected file name
    dc.SetFont(GetFont());

    dc.SetPen(wxPen(theme_->GetColor(SELECT_BORDER)));
    dc.SetBrush(wxBrush(theme_->GetColor(SELECT_BG)));
    dc.SetTextForeground(theme_->GetColor(SELECT_FG));

    wxRect text_rect = text_rects_[select_index_];
    dc.DrawRectangle(text_rect);

    text_rect.Deflate(kPaddingX, kPaddingY);
    ui::DrawTextInRect(dc, text_pages_[select_index_]->Page_Label(), text_rect);

    // Selected file path
    dc.SetTextForeground(theme_->GetColor(FG));

    wxString path = text_pages_[select_index_]->Page_Description();
    ui::DrawTextInRect(dc, path, path_rect_);
  }

  dc.SetFont(GetFont());
  dc.SetTextForeground(theme_->GetColor(FG));

  for (size_t i = 0; i < text_pages_.size(); ++i) {
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
  size_t index = GetIndexByPos(evt.GetPosition());

  if (index != editor::kNpos) {
    select_index_ = index;
    Close();
  } else {
    evt.Skip();
  }
}

void NavigationDialog::OnMouseMotion(wxMouseEvent& evt) {
  size_t index = GetIndexByPos(evt.GetPosition());

  if (index != editor::kNpos) {
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
  int w = column_width_ * cols + kSpaceX * (cols - 1);

  // Client height without margin.
  int h = title_h + kSpaceY +
          name_h * max_rows_ + kSpaceY +
          path_h;

  SetClientSize(w + kMarginX + kMarginX, h + kMarginY + kMarginY);

  title_rect_ = wxRect(kMarginX, kMarginY, w, title_h);
  path_rect_ = wxRect(kMarginX, kMarginY + h - path_h, w, path_h);

  for (size_t i = 0; i < text_pages_.size(); ++i) {
    int col = i / max_rows_;
    int row = i % max_rows_;

    int x = kMarginX + (column_width_ + kSpaceX) * col;
    int y = kMarginY + title_h + kSpaceY + name_h * row;
    text_rects_[i] = wxRect(x, y, column_width_, name_h);
  }
}

size_t NavigationDialog::GetIndexByPos(const wxPoint& pos) const {
  for (size_t i = 0; i < text_rects_.size(); ++i) {
    if (text_rects_[i].Contains(pos)) {
      return i;
    }
  }
  return editor::kNpos;
}

}  // namespace jil
