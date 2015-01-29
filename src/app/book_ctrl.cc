#include "app/book_ctrl.h"

#include <cassert>
#include <memory>

#ifdef __WXMSW__
#if JIL_BOOK_USE_GDIPLUS
#include <objidl.h>
#include <gdiplus.h>
#include "wx/msw/private.h"
#endif  // JIL_BOOK_USE_GDIPLUS
#endif  // __WXMSW__

#include "wx/sizer.h"
#include "wx/graphics.h"
#include "wx/dcgraph.h"
#include "wx/menu.h"
#include "wx/wupdlock.h"
#include "wx/log.h"

#include "base/math_util.h"

#include "editor/text_extent.h"
#if !JIL_BOOK_NATIVE_TOOLTIP
#include "editor/tip.h"
#endif

#include "app/i18n_strings.h"
#include "app/id.h"

#define JIL_TAB_ROUND_CORNER 0

namespace jil {

const int kFadeAwayChars = 3;

////////////////////////////////////////////////////////////////////////////////

class BookTabArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  BookTabArea(BookCtrl* book_ctrl, wxWindowID id)
      : wxPanel(book_ctrl, id)
      , book_ctrl_(book_ctrl)
#if !JIL_BOOK_NATIVE_TOOLTIP
      , tip_handler_(NULL)
#endif
  {
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetCursor(wxCursor(wxCURSOR_ARROW));
  }

  virtual ~BookTabArea() {
#if !JIL_BOOK_NATIVE_TOOLTIP
    if (tip_handler_ != NULL) {
      PopEventHandler();
      delete tip_handler_;
    }
#endif  // !JIL_BOOK_NATIVE_TOOLTIP
  }

  // Page header has no focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

  void SetToolTipEx(const wxString& tooltip) {
#if JIL_BOOK_NATIVE_TOOLTIP
    SetToolTip(tooltip);
#else
    if (tip_handler_ == NULL) {
      tip_handler_ = new editor::TipHandler(this);
      tip_handler_->set_start_on_move(true);
      PushEventHandler(tip_handler_);
    }
    tip_handler_->SetTip(tooltip);
#endif  // JIL_BOOK_NATIVE_TOOLTIP
  }

protected:
  virtual wxSize DoGetBestSize() const {
    return book_ctrl_->CalcTabAreaBestSize();
  }

  void OnSize(wxSizeEvent& evt) {
    book_ctrl_->OnTabSize(evt);
  }

  void OnPaint(wxPaintEvent& evt) {
    wxAutoBufferedPaintDC dc(this);

#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
    dc.SetBackground(GetBackgroundColour());
    dc.Clear();
#endif

    book_ctrl_->OnTabPaint(dc, evt);
  }

  void OnMouseEvents(wxMouseEvent& evt) {
    book_ctrl_->OnTabMouse(evt);
  }

  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
    // Do nothing.
  }

private:
  BookCtrl* book_ctrl_;

#if !JIL_BOOK_NATIVE_TOOLTIP
  editor::TipHandler* tip_handler_;
#endif
};

BEGIN_EVENT_TABLE(BookTabArea, wxPanel)
EVT_SIZE(BookTabArea::OnSize)
EVT_PAINT(BookTabArea::OnPaint)
EVT_MOUSE_EVENTS(BookTabArea::OnMouseEvents)
EVT_MOUSE_CAPTURE_LOST(BookTabArea::OnMouseCaptureLost)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

BookCtrl::BookCtrl(const editor::SharedTheme& theme)
    : theme_(theme) {
  Init();
}

BookCtrl::~BookCtrl() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    Disconnect((*it)->page->Page_Window()->GetId());
    delete (*it);
  }
  tabs_.clear();
}

bool BookCtrl::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(BG));

  CreateTabArea();

  // Book ctrl's min size is the best size of its tab area.
  SetMinSize(tab_area_->GetBestSize());

  page_area_ = new wxPanel(this, wxID_ANY);
  page_vsizer_ = new wxBoxSizer(wxVERTICAL);
  page_area_->SetSizer(page_vsizer_);

  wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
  vsizer->Add(tab_area_, 0, wxEXPAND);
  vsizer->Add(page_area_, 1, wxEXPAND);

  SetSizer(vsizer);

  return true;
}

bool BookCtrl::HasFocus() const {
  if (wxPanel::HasFocus()) {
    return true;
  }

  BookPage* page = ActivePage();
  if (page != NULL && page->Page_Window()->HasFocus()) {
    return true;
  }

  return false;
}

void BookCtrl::StartBatch() {
  batch_ = true;
  Freeze();
}

void BookCtrl::EndBatch() {
  batch_ = false;
  Thaw();

  if (need_resize_tabs_) {
    ResizeTabs(true);
    need_resize_tabs_ = false;
  }
}

// TODO: No return value.
bool BookCtrl::AddPage(BookPage* page, bool active) {
  Tab* tab = new Tab(page, CalcTabBestSize(page->Page_Label()));
  tabs_.push_back(tab);

  // Try to avoid resizing tabs.
  int expected_size = tab->best_size > tab_default_size_ ?
      tab->best_size : tab_default_size_;
  if (expected_size <= free_size_) {
    tab->size = expected_size;
    free_size_ -= expected_size;
  } else {
    if (!batch_) {
      ResizeTabs();
    } else {
      need_resize_tabs_ = true;
    }
  }

  if (active) {
    ActivatePage(--tabs_.end());
  } else {
    stack_tabs_.push_back(tab);
  }

  PostEvent(kEvtBookPageChange);

  return true;
}

bool BookCtrl::RemovePage(const BookPage* page) {
  TabList::iterator it = TabByPage(page);
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool BookCtrl::RemoveActivePage() {
  TabList::iterator it = ActiveTab();
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool BookCtrl::RemoveAllPages(const BookPage* except_page) {
  wxWindowUpdateLocker avoid_flickering(this);

  bool any_page_removed = false;

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ) {
    Tab* tab = *it;
    if (tab->page == except_page) {
      ++it;
      continue;
    }

    if (!tab->page->Page_Close()) {
      // Failed to close page, stop removing.
      break;
    }

    // Reset right-clicked tab since the page will be destroyed.
    if (rclicked_tab_ != NULL && rclicked_tab_->page == tab->page) {
      rclicked_tab_ = NULL;
    }

    it = tabs_.erase(it);
    stack_tabs_.remove(tab);
    if (tab->active) {
      page_vsizer_->Clear(false);
    }
    delete tab;

    any_page_removed = true;
  }

  if (any_page_removed) {
    if (!stack_tabs_.empty()) {
      ActivatePage(stack_tabs_.front()->page);

      PostEvent(kEvtBookPageSwitch);
    }

    // The tab is removed, more space is available, resize the left tabs.
    ResizeTabs(false);

    tab_area_->Refresh();

    PostEvent(kEvtBookPageChange);
  }

  return (PageCount() == (except_page == NULL ? 0 : 1));
}

void BookCtrl::ActivatePage(BookPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      ActivatePage(it);
      return;
    }
  }
}

BookPage* BookCtrl::ActivePage() const {
  if (stack_tabs_.empty()) {
    return NULL;
  }
  if (!stack_tabs_.front()->active) {
    return NULL;
  }
  return stack_tabs_.front()->page;
}

void BookCtrl::SwitchToNextPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      ++it;
      if (it == tabs_.end()) {
        it = tabs_.begin();
      }
      ActivatePage(it);
      return;
    }
  }
}

void BookCtrl::SwitchToPrevPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      if (it == tabs_.begin()) {
        it = tabs_.end();
      }
      --it;
      ActivatePage(it);
      return;
    }
  }
}

#if 0
void BookCtrl::SwitchToNextStackPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = stack_tabs_.begin();
  ++it;
  ActivatePage((*it)->page);
}

void BookCtrl::SwitchToPrevStackPage() {
  if (PageCount() <= 1) {
    return;
  }

  ActivatePage(stack_tabs_.back()->page);
}
#endif

std::vector<BookPage*> BookCtrl::Pages() const {
  std::vector<BookPage*> pages;

  TabList::const_iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    pages.push_back((*it)->page);
  }

  return pages;
}

std::vector<BookPage*> BookCtrl::StackPages() const {
  std::vector<BookPage*> pages;

  TabList::const_iterator it = stack_tabs_.begin();
  for (; it != stack_tabs_.end(); ++it) {
    pages.push_back((*it)->page);
  }

  return pages;
}

BookPage* BookCtrl::NextPage(const BookPage* page) const {
  if (tabs_.size() <= 1) {
    return NULL;
  }

  TabList::const_iterator it = TabByPage(page);
  if (it == tabs_.end()) {
    return NULL;
  }

  ++it;
  if (it == tabs_.end()) {
    return tabs_.front()->page;
  } else {
    return (*it)->page;
  }
}

void BookCtrl::ResizeTabs(bool refresh) {
  if (tabs_.empty()) {
    return;
  }

  free_size_ = 0;  // Reset

  int client_size = tab_area_->GetClientSize().x;
  client_size -= tab_area_padding_x_ + tab_area_padding_x_;

  int sum_prev_size = 0;
  int sum_best_size = 0;

  for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
    sum_prev_size += (*it)->size;
    sum_best_size += (*it)->best_size;

    // Reset tab size to its best size.
    (*it)->size = (*it)->best_size;
  }

  if (sum_best_size < client_size) {
    // Give more size to small tabs.

    TabList small_tabs;
    for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
      if ((*it)->size < tab_default_size_) {
        small_tabs.push_back(*it);
      }
    }

    int free_size = client_size - sum_best_size;

    while (!small_tabs.empty()) {
      int avg_free_size = free_size / static_cast<int>(small_tabs.size());
      free_size %= static_cast<int>(small_tabs.size());

      if (avg_free_size == 0) {
        // Give 1px to each small tab.
        for (TabIter it = small_tabs.begin(); it != small_tabs.end(); ++it) {
          if (free_size == 0) {
            break;
          }
          ++(*it)->size;
          --free_size;
        }
        break;
      }

      for (TabIter it = small_tabs.begin(); it != small_tabs.end(); ) {
        Tab* small_tab = *it;
        int needed_size = tab_default_size_ - small_tab->size;
        if (needed_size > avg_free_size) {
          small_tab->size += avg_free_size;
          ++it;
        } else {
          // This tab doesn't need that much size.
          small_tab->size = tab_default_size_;
          // Return extra free size back.
          free_size += avg_free_size - needed_size;
          // This tab is not small any more.
          it = small_tabs.erase(it);
        }
      }
    }  // while (!small_tabs.empty())

    // Save free size.
    free_size_ = free_size;
    wxLogDebug(wxT("ResizeTabs, free_size: %d"), free_size_);

  } else {  // sum_best_size >= client_size
    // Reduce the size of large tabs (except the active one).

    int lack_size = sum_best_size - client_size;

    for (int large_size = tab_default_size_;
         large_size > tab_min_size_ && lack_size > 0;
         large_size /= 2) {
      TabList large_tabs;
      for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
        Tab* large_tab = *it;
        if (large_tab->size > large_size && !large_tab->active) {
          large_tabs.push_back(large_tab);
        }
      }

      if (large_tabs.empty()) {
        continue;
      }

      while (!large_tabs.empty()) {
        int avg_lack_size = lack_size / large_tabs.size();
        lack_size %= large_tabs.size();

        if (avg_lack_size == 0) {
          // Take 1px from first "lack_size" number of large tabs.
          for (TabIter it = large_tabs.begin(); it != large_tabs.end(); ++it) {
            if (lack_size == 0) {
              break;
            }
            --(*it)->size;
            --lack_size;
          }
          break;
        }

        for (TabIter it = large_tabs.begin(); it != large_tabs.end(); ) {
          Tab* large_tab = *it;
          int givable_size = large_tab->size - large_size;
          if (givable_size > avg_lack_size) {
            large_tab->size -= avg_lack_size;
            ++it;
          } else {
            // This tab cannot give that much size. Give all it can give.
            large_tab->size = large_size;
            // Return extra lack size back.
            lack_size += avg_lack_size - givable_size;
            // This tab is not large any more.
            it = large_tabs.erase(it);
          }
        }
      }
    }
  }

  if (refresh) {
    // Refresh if the size of tabs is changed.
    int sum_size = 0;
    for (TabIter it = tabs_.begin(); it != tabs_.end(); ++it) {
      sum_size += (*it)->size;
    }

    if (sum_size != sum_prev_size) {
      RefreshTabArea();
    }
  }
}

void BookCtrl::Init() {
  tab_margin_top_ = 3;
  tab_area_padding_x_ = 3;

  tab_min_size_ = 10;
  tab_default_size_ = 130;

  free_size_ = 0;
  rclicked_tab_ = NULL;
  batch_ = false;
  need_resize_tabs_ = false;
}

void BookCtrl::CreateTabArea() {
  tab_area_ = new BookTabArea(this, wxID_ANY);

  if (theme_->GetColor(TAB_AREA_BG).IsOk()) {
    tab_area_->SetBackgroundColour(theme_->GetColor(TAB_AREA_BG));
  }

  if (theme_->GetFont(TAB_FONT).IsOk()) {
    tab_area_->SetFont(theme_->GetFont(TAB_FONT));
  }

  // Use char width as the padding.
  int char_width = tab_area_->GetCharWidth();
  tab_padding_.Set(char_width, char_width / 2 + 1);
}

void BookCtrl::OnTabSize(wxSizeEvent& evt) {
  if (!batch_) {
    ResizeTabs();
  }
  evt.Skip();
}

#ifdef __WXMSW__
#if JIL_BOOK_USE_GDIPLUS

static inline Gdiplus::Color GdiplusColor(const wxColour& wx_color) {
  return Gdiplus::Color(wx_color.Alpha(),
                        wx_color.Red(),
                        wx_color.Green(),
                        wx_color.Blue());
}

static inline Gdiplus::Rect GdiplusRect(const wxRect& wx_rect) {
  return Gdiplus::Rect(wx_rect.x, wx_rect.y, wx_rect.width, wx_rect.height);
}

static Gdiplus::Font* GdiplusNewFont(const wxFont& wx_font) {
  int style = Gdiplus::FontStyleRegular;

  if (wx_font.GetStyle() == wxFONTSTYLE_ITALIC) {
    style |= Gdiplus::FontStyleItalic;
  }
  if (wx_font.GetUnderlined()) {
    style |= Gdiplus::FontStyleUnderline;
  }
  if (wx_font.GetWeight() == wxFONTWEIGHT_BOLD) {
    style |= Gdiplus::FontStyleBold;
  }

  return new Gdiplus::Font(wx_font.GetFaceName().wc_str(),
                           wx_font.GetPointSize(),
                           style,
                           Gdiplus::UnitPoint);
}

#endif  // JIL_BOOK_USE_GDIPLUS
#endif  // __WXMSW__

// Paint tab items.
void BookCtrl::OnTabPaint(wxAutoBufferedPaintDC& dc, wxPaintEvent& evt) {
#ifdef __WXMAC__
  wxGraphicsContext* gc = dc.GetGraphicsContext();
#else
  wxGCDC gcdc(dc);
  wxGraphicsContext* gc = gcdc.GetGraphicsContext();
#endif  // __WXMAC__

  DrawBackground(gc);
  DrawForeground(gc, dc);
}

void BookCtrl::DrawTabBackground(Tab* tab, wxGraphicsContext* gc, int x) {
  wxRect rect = tab_area_->GetClientRect();

  wxColour tab_border;
  wxColour tab_bg;

  if (tab->active) {
    tab_border = theme_->GetColor(ACTIVE_TAB_BORDER);
    tab_bg = theme_->GetColor(ACTIVE_TAB_BG);
  } else {
    tab_border = theme_->GetColor(TAB_BORDER);
    tab_bg = theme_->GetColor(TAB_BG);
  }

  // TODO: Check in DrawBackground.
  // If the tab border and bg are the same color as the tab area bg,
  // don't have to draw.
  if (!tab_border.IsOk() && !tab_bg.IsOk() ||
      tab_border == tab_bg && tab_bg == theme_->GetColor(TAB_AREA_BG)) {
    return;
  }

  gc->SetPen(wxPen(tab_border));
  gc->SetBrush(wxBrush(tab_bg));

  wxRect tab_rect(x + 1,
                  rect.GetTop() + tab_margin_top_,
                  tab->size - 1,
                  rect.GetHeight() - tab_margin_top_ + 1);

  wxGraphicsPath border_path = gc->CreatePath();
  border_path.MoveToPoint(tab_rect.GetLeft(), tab_rect.GetBottom());

#if JIL_TAB_ROUND_CORNER
  const wxDouble kRadius = 4.0f;
  // Top left corner.
  border_path.AddArc(tab_rect.GetLeft() + kRadius,
                     tab_rect.GetTop() + kRadius,
                     kRadius,
                     base::kRadian180,
                     base::kRadian270,
                     true);
  // Top line.
  border_path.AddLineToPoint(tab_rect.GetRight() - kRadius, tab_rect.GetTop());
  // Top right corner.
  border_path.AddArc(tab_rect.GetRight() - kRadius,
                     tab_rect.GetTop() + kRadius,
                     kRadius,
                     base::kRadian270,
                     base::kRadian0,
                     true);
  // Right line.
  border_path.AddLineToPoint(tab_rect.GetRight(), tab_rect.GetBottom());
#else
  border_path.AddLineToPoint(tab_rect.GetLeft(), tab_rect.GetTop());
  border_path.AddLineToPoint(tab_rect.GetRight(), tab_rect.GetTop());
  border_path.AddLineToPoint(tab_rect.GetRight(), tab_rect.GetBottom());
#endif  // JIL_NOOTBOOK_TAB_ROUND_CORNER

  gc->DrawPath(border_path);
}

void BookCtrl::DrawBackground(wxGraphicsContext* gc) {
  wxRect rect = tab_area_->GetClientRect();

  int x = rect.GetLeft() + tab_area_padding_x_;

  // Draw bottom line to outline the active tab.
  wxColour active_tab_border = theme_->GetColor(ACTIVE_TAB_BORDER);
  if (active_tab_border.IsOk()) {
    gc->SetPen(wxPen(active_tab_border));

    int y = rect.GetBottom();
    gc->StrokeLine(rect.GetLeft(), y, rect.GetRight() + 1, y);
  }

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it) {
    DrawTabBackground(*it, gc, x);
    x += (*it)->size;
  }
}

void BookCtrl::DrawForeground(wxGraphicsContext* gc, wxDC& dc) {
  wxRect rect = tab_area_->GetClientRect();

#ifdef __WXMSW__
#if JIL_BOOK_USE_GDIPLUS
  std::auto_ptr<Gdiplus::Graphics> graphics;
  std::auto_ptr<Gdiplus::Font> gdip_font;
#endif  // JIL_BOOK_USE_GDIPLUS
#endif  // __WXMSW__

  int x = rect.GetLeft() + tab_area_padding_x_;

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it) {
    Tab* tab = *it;

    ColorId fg_color_id = tab->active ? ACTIVE_TAB_FG : TAB_FG;
    const wxColour& tab_fg = theme_->GetColor(fg_color_id);

    gc->SetFont(tab_area_->GetFont(), tab_fg);

    wxRect tab_rect(x + tab_padding_.x,
                    rect.GetTop() + tab_padding_.y + tab_margin_top_,
                    tab->size - tab_padding_.x - tab_padding_.x,
                    rect.GetHeight() - tab_padding_.y - tab_margin_top_);

    if (tab_rect.GetWidth() > 0) {
      if (tab->best_size <= tab->size) {
        // Enough space to display the label completely.
        gc->DrawText(tab->page->Page_Label(),
                     tab_rect.GetLeft(),
                     tab_rect.GetTop());
      } else {
        // No enough space.
        wxString label = tab->page->Page_Label();

        // Fit label to the given space.
        label = label.Mid(0,
                          editor::TailorLabel(dc, label, tab_rect.GetWidth()));

        wxDouble fade_x = tab_rect.GetLeft();

        if (label.size() > kFadeAwayChars) {
          wxString label_before = label.Mid(0, label.size() - kFadeAwayChars);
          gc->DrawText(label_before, tab_rect.GetLeft(), tab_rect.GetTop());

          wxDouble label_before_width = 0.0f;
          gc->GetTextExtent(label_before,
                            &label_before_width,
                            NULL,
                            NULL,
                            NULL);
          fade_x += label_before_width;

          label = label.Mid(label.size() - kFadeAwayChars);
        }

        // Fade away the last several characters.

        wxDouble label_width = 0.0f;
        gc->GetTextExtent(label, &label_width, NULL, NULL, NULL);

#ifdef __WXMSW__
#if JIL_BOOK_USE_GDIPLUS
        // Lazily create GDI+ objects.
        if (graphics.get() == NULL) {
          graphics.reset(new Gdiplus::Graphics((HDC)dc.GetHDC()));
          gdip_font.reset(GdiplusNewFont(tab_area_->GetFont()));
        }

        ColorId tab_bg_id = tab->active ? ACTIVE_TAB_BG : TAB_BG;
        const wxColour& tab_bg = theme_->GetColor(tab_bg_id);

        std::auto_ptr<Gdiplus::Brush> brush(new Gdiplus::LinearGradientBrush(
          GdiplusRect(wxRect(fade_x,
                             tab_rect.GetTop(),
                             std::ceil(label_width),
                             tab_rect.GetHeight())),
          GdiplusColor(tab_fg),
          GdiplusColor(tab_bg),
          Gdiplus::LinearGradientModeHorizontal));

        graphics->DrawString(label.wc_str(*wxConvUI),
                             -1,
                             gdip_font.get(),
                             Gdiplus::PointF(fade_x, tab_rect.GetTop()),
                             Gdiplus::StringFormat::GenericTypographic(),
                             brush.get());
#else
        gc->DrawText(label, fade_x, tab_rect.GetTop());

#endif  // JIL_BOOK_USE_GDIPLUS
#else
        // TODO: Fade away.
        gc->DrawText(label, fade_x, tab_rect.GetTop());
#endif  // __WXMSW__
      }
    }

    // Modified indicator.
    if ((tab->page->Page_Flags() & BookPage::kModified) != 0) {
      dc.SetTextForeground(tab_fg);
      dc.DrawText(wxT("*"), tab_rect.GetRight() + 5, tab_rect.GetTop());
    }

    x += tab->size;
  }
}

void BookCtrl::OnTabMouse(wxMouseEvent& evt) {
  wxEventType evt_type = evt.GetEventType();

  if (evt_type == wxEVT_LEFT_DOWN) {
    OnTabMouseLeftDown(evt);
  } else if (evt_type == wxEVT_LEFT_UP) {
    OnTabMouseLeftUp(evt);
  } else if (evt_type == wxEVT_MOTION) {
    OnTabMouseMotion(evt);
  } else if (evt_type == wxEVT_RIGHT_DOWN) {
    OnTabMouseRightDown(evt);
  } else if (evt_type == wxEVT_RIGHT_UP) {
    OnTabMouseRightUp(evt);
  } else if (evt_type == wxEVT_MIDDLE_DOWN) {
    OnTabMouseMiddleDown(evt);
  } else if (evt_type == wxEVT_MIDDLE_UP) {
    OnTabMouseMiddleUp(evt);
  } else if (evt_type == wxEVT_LEFT_DCLICK) {
    OnTabMouseLeftDClick(evt);
  }

  evt.Skip();
}

void BookCtrl::OnTabMouseLeftDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseLeftDown(evt);
}

void BookCtrl::HandleTabMouseLeftDown(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseLeftUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }

  HandleTabMouseLeftUp(evt);

  tab_area_->ReleaseMouse();
}

void BookCtrl::HandleTabMouseLeftUp(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseMiddleDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }
}

void BookCtrl::OnTabMouseMiddleUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }
  tab_area_->ReleaseMouse();

  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    return;
  }

  RemovePage(it);
}

// Update tooltip on mouse motion.
void BookCtrl::OnTabMouseMotion(wxMouseEvent& evt) {
  wxString tooltip;

  TabList::iterator it = TabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    tooltip = (*it)->page->Page_Description();
  }

  tab_area_->SetToolTipEx(tooltip);
}

void BookCtrl::OnTabMouseRightDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }

  // Without this the active page won't have the focus.
  if (!HasFocus()) {
    SetFocus();
  }

  HandleTabMouseRightDown(evt);
}

void BookCtrl::HandleTabMouseRightDown(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseRightUp(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    return;
  }
  tab_area_->ReleaseMouse();

  HandleTabMouseRightUp(evt);

  rclicked_tab_ = NULL;
}

void BookCtrl::HandleTabMouseRightUp(wxMouseEvent& evt) {
}

void BookCtrl::OnTabMouseLeftDClick(wxMouseEvent& evt) {
  HandleTabMouseLeftDClick(evt);
}

void BookCtrl::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
}

BookCtrl::TabList::iterator BookCtrl::TabByPos(int pos_x) {
  int x = 0;

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if (pos_x >= x && pos_x < x + (*it)->size) {
      return it;
    }

    x += (*it)->size;
  }

  return tabs_.end();
}

BookCtrl::Tab* BookCtrl::GetTabByWindow(wxWindow* window, size_t* index) {
  size_t i = 0;

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it, ++i) {
    if ((*it)->page->Page_Window() == window) {
      if (index != NULL) {
        *index = i;
      }
      return (*it);
    }
  }

  return NULL;
}

void BookCtrl::ActivatePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return;
  }

  Tab* tab = *it;
  if (tab->active) {
    return;  // Already active
  }

  page_area_->Freeze();

  // Deactivate previous active page.
  TabList::iterator active_it = ActiveTab();
  if (active_it != tabs_.end()) {
    (*active_it)->active = false;
    (*active_it)->page->Page_Activate(false);
    page_vsizer_->Clear(false);
  }

  // Activate new page.
  (*it)->active = true;
  page_vsizer_->Add((*it)->page->Page_Window(), 1, wxEXPAND);
  (*it)->page->Page_Activate(true);

  // Make sure the active tab has enough space to display.
  if ((*it)->size < (*it)->best_size) {
    ResizeTabs();
  }

  // Update tab stack.
  stack_tabs_.remove(tab);
  stack_tabs_.push_front(tab);

  page_area_->Layout();
  page_area_->Thaw();

  tab_area_->Refresh();

  PostEvent(kEvtBookPageSwitch);
}

bool BookCtrl::RemovePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;

  if (!tab->page->Page_Close()) {
    // Failed to close page, stop removing.
    return false;
  }

  // Reset right-clicked tab since the page will be destroyed.
  if (rclicked_tab_ != NULL && rclicked_tab_->page == tab->page) {
    rclicked_tab_ = NULL;
  }

  page_area_->Freeze();

  tabs_.erase(it);
  stack_tabs_.remove(tab);

  // The page to remove is active.
  if (tab->active) {
    page_vsizer_->Clear(false);

    if (PageCount() > 0) {
      // Activate another page.
      Tab* active_tab = stack_tabs_.front();
      active_tab->active = true;
      page_vsizer_->Add(active_tab->page->Page_Window(), 1, wxEXPAND);
      active_tab->page->Page_Activate(true);

      PostEvent(kEvtBookPageSwitch);
    }
  }

  delete tab;

  page_area_->Layout();
  page_area_->Thaw();

  // Resize tabs since more space is available.
  ResizeTabs(false);
  tab_area_->Refresh();

  PostEvent(kEvtBookPageChange);

  return true;
}

BookCtrl::TabList::iterator BookCtrl::ActiveTab() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      break;
    }
  }
  return it;
}

BookCtrl::TabList::iterator BookCtrl::TabByPage(const BookPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

BookCtrl::TabList::const_iterator BookCtrl::TabByPage(
    const BookPage* page) const {
  TabList::const_iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

int BookCtrl::CalcTabBestSize(const wxString& label) const {
  int label_size = 0;
  tab_area_->GetTextExtent(label, &label_size, NULL);
  return label_size + tab_padding_.x + tab_padding_.x;
}

wxSize BookCtrl::CalcTabAreaBestSize() const {
  int y = tab_area_->GetCharHeight();
  y += tab_margin_top_ + tab_padding_.y + tab_padding_.y;
  return wxSize(-1, y);
}

void BookCtrl::RefreshTabArea() {
  tab_area_->Refresh();
}

void BookCtrl::PostEvent(wxEventType event_type) {
  wxCommandEvent evt(event_type, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

////////////////////////////////////////////////////////////////////////////////
// Define event types.

DEFINE_EVENT_TYPE(kEvtBookPageChange);
DEFINE_EVENT_TYPE(kEvtBookPageSwitch);

}  // namespace jil
