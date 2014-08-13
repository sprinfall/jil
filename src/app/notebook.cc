#include "app/notebook.h"

#include <cassert>
#include <memory>

#ifdef __WXMSW__
#define USE_GDIPLUS_DIRECTLY 1
#if USE_GDIPLUS_DIRECTLY
#include <objidl.h>
#include <gdiplus.h>
#include "wx/msw/private.h"
#endif // USE_GDIPLUS_DIRECTLY
#endif // __WXMSW__

#include "wx/sizer.h"
#include "wx/graphics.h"
#include "wx/dcgraph.h"
#include "wx/menu.h"
#include "wx/wupdlock.h"
#include "wx/log.h"

#include "base/foreach.h"
#include "base/math_util.h"
#include "base/compiler_specific.h"

#include "editor/text_extent.h"
#if !JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
#include "editor/tip.h"
#endif

#include "app/text_page.h"
#include "app/i18n_strings.h"
#include "app/id.h"

#define JIL_NOOTBOOK_USE_NATIVE_TOOLTIP 0

// Right click floating menu labels.
#define kTrRClickSave _("Save")
#define kTrRClickClose _("Close")
#define kTrRClickCloseAll _("Close All")
#define kTrRClickCloseAllButThis _("Close All But This")
#define kTrRClickCloseAllButThis _("Close All But This")
#define kTrRClickCopyFilePath _("Copy File Path")
#ifdef __WXMAC__
#define kTrRClickOpenFolder _("View In Finder")
#else
#define kTrRClickOpenFolder _("Open Containing Folder")
#endif
#define kTrRClickNewFile ("New File")

namespace jil {\

namespace notebook {\

////////////////////////////////////////////////////////////////////////////////

static const int kMarginTop = 5;
static const int kTabPaddingLeft = 5;
static const int kTabPaddingX = 20;
static const int kTabPaddingTop = 7;
static const int kTabPaddingBottom = 7;

const int kFadeAwayChars = 3;

class TabArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  TabArea(Notebook* notebook, wxWindowID id)
      : wxPanel(notebook, id)
      , notebook_(notebook)
#if !JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
      , tip_handler_(NULL)
#endif
  {
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
  }

  virtual ~TabArea() {
#if !JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
    if (tip_handler_ != NULL) {
      PopEventHandler();
      delete tip_handler_;
    }
#endif // !JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
  }

  // Page header has no focus.
  virtual bool AcceptsFocus() const OVERRIDE { return false; }

  void SetToolTipEx(const wxString& tooltip) {
#if JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
    SetToolTip(tooltip);
#else
    if (tip_handler_ == NULL) {
      tip_handler_ = new editor::TipHandler(this);
      tip_handler_->set_start_on_move(true);
      PushEventHandler(tip_handler_);
    }
    tip_handler_->SetTip(tooltip);
#endif // JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
  }

protected:
  virtual wxSize DoGetBestSize() const {
    int y = 0;
    GetTextExtent(wxT("T"), NULL, &y, 0, 0);
    return wxSize(-1, y + kMarginTop + kTabPaddingTop + kTabPaddingBottom);
  }

  void OnSize(wxSizeEvent& evt) {
    notebook_->OnTabSize(evt);
  }

  void OnPaint(wxPaintEvent& evt) {
    wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
    dc.SetBackground(GetBackgroundColour());
    dc.Clear();
#endif

    notebook_->OnTabPaint(dc, evt);
  }

  void OnMouseEvents(wxMouseEvent& evt) {
    notebook_->OnTabMouse(evt);
  }

  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
    // Do nothing.
  }

private:
  Notebook* notebook_;

#if !JIL_NOOTBOOK_USE_NATIVE_TOOLTIP
  editor::TipHandler* tip_handler_;
#endif
};

BEGIN_EVENT_TABLE(TabArea, wxPanel)
  EVT_SIZE                (TabArea::OnSize)
  EVT_PAINT               (TabArea::OnPaint)
  EVT_MOUSE_EVENTS        (TabArea::OnMouseEvents)
  EVT_MOUSE_CAPTURE_LOST  (TabArea::OnMouseCaptureLost)
END_EVENT_TABLE()

} // namespace notebook

////////////////////////////////////////////////////////////////////////////////

using namespace notebook;

BEGIN_EVENT_TABLE(Notebook, wxPanel)
END_EVENT_TABLE()

Notebook::Notebook(const editor::SharedTheme& theme)
    : theme_(theme)
    , tab_min_size_(10)
    , tab_default_size_(130)
    , free_size_(0)
    , rclicked_tab_(NULL)
    , batch_(false)
    , need_resize_tabs_(false) {
}

bool Notebook::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundColour(theme_->GetColor(BG));

  tab_area_ = new TabArea(this, wxID_ANY);
  if (theme_->GetColor(TAB_AREA_BG).IsOk()) {
    tab_area_->SetBackgroundColour(theme_->GetColor(TAB_AREA_BG));
  }
  if (theme_->GetFont(TAB_FONT).IsOk()) {
    tab_area_->SetFont(theme_->GetFont(TAB_FONT));
  }

  page_area_ = new PageArea(this, wxID_ANY);
  page_sizer_ = new wxBoxSizer(wxVERTICAL);
  page_area_->SetSizer(page_sizer_);

  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(tab_area_, 0, wxEXPAND);
  sizer->Add(page_area_, 1, wxEXPAND);

  SetSizer(sizer);

  return true;
}

Notebook::~Notebook() {
  foreach (Tab* tab, tabs_) {
    Disconnect(tab->page->GetId());
    delete tab;
  }
  tabs_.clear();
}

void Notebook::StartBatch() {
  batch_ = true;
  Freeze();
}

void Notebook::EndBatch() {
  batch_ = false;
  Thaw();

  if (need_resize_tabs_) {
    ResizeTabs(/*refresh=*/true);
    need_resize_tabs_ = false;
  }
}

bool Notebook::AddPage(TextPage* page, bool active/*=false*/) {
  Tab* tab = new Tab(page, CalcTabBestSize(page->NPage_GetLabel()));
  tabs_.push_back(tab);

  // Try to avoid resizing tabs.
  int expected_size = tab->best_size > tab_default_size_ ? tab->best_size : tab_default_size_;
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
    active_age_list_.push_back(tab);
  }

  // Connect event handlers.
  Connect(page->GetId(),
          editor::kEvtTextFileNameChange,
          wxCommandEventHandler(Notebook::OnTextFileNameChange));
  Connect(page->GetId(),
          editor::kEvtTextModifiedChange,
          wxCommandEventHandler(Notebook::OnTextModifiedChange));

  return true;
}

bool Notebook::RemovePage(const TextPage* page) {
  TabList::iterator it = GetTabByPage(page);
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool Notebook::RemoveActivePage() {
  TabList::iterator it = GetActiveTab();
  if (it == tabs_.end()) {
    return false;
  }

  return RemovePage(it);
}

bool Notebook::RemoveAllPages(const TextPage* except_page) {
  wxWindowUpdateLocker avoid_flickering(this);

  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ) {
    Tab* tab = *it;
    if (tab->page == except_page) {
      ++it;
      continue;
    }

    if (!tab->page->NPage_Close()) {
      // Failed to close page, stop removing.
      break;
    }

    // Reset right-clicked tab since the page will be destroyed.
    if (rclicked_tab_ != NULL && rclicked_tab_->page == tab->page) {
      rclicked_tab_ = NULL;
    }

    it = tabs_.erase(it);
    active_age_list_.remove(tab);
    if (tab->active) {
      page_sizer_->Clear(false);
    }
    tab->page->Destroy();
    delete tab;
  }

  if (!active_age_list_.empty()) {
    ActivatePage(active_age_list_.front()->page);
  }

  tab_area_->Refresh();

  return (PageCount() == (except_page == NULL ? 0 : 1));
}

void Notebook::ActivatePage(TextPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      ActivatePage(it);
      return;
    }
  }
}

TextPage* Notebook::GetActivePage() const {
  if (active_age_list_.empty()) {
    return NULL;
  }
  if (!active_age_list_.front()->active) {
    return NULL;
  }
  return active_age_list_.front()->page;
}

void Notebook::SwitchPage() {
  if (PageCount() <= 1) {
    return;
  }

  TabList::iterator it = active_age_list_.begin();
  ++it;
  ActivatePage((*it)->page);
}

std::vector<TextPage*> Notebook::GetPages() const {
  std::vector<TextPage*> pages;

  foreach (Notebook::Tab* tab, tabs_) {
    pages.push_back(tab->page);
  }

  return pages;
}

void Notebook::ResizeTabs(bool refresh/*=true*/) {
  if (tabs_.empty()) {
    return;
  }

  wxLogDebug(wxT("Notebook::ResizeTabs"));

  free_size_ = 0; // Reset

  const int client_size = tab_area_->GetClientSize().GetWidth();

  int sum_prev_size = 0;
  int sum_best_size = 0;
  foreach (Tab* tab, tabs_) {
    sum_prev_size += tab->size;
    sum_best_size += tab->best_size;
    // Reset size.
    tab->size = tab->best_size;
  }

  if (sum_best_size < client_size) {
    // Give more size to small tabs.

    TabList small_tabs;
    foreach (Tab* tab, tabs_) {
      if (tab->size < tab_default_size_) {
        small_tabs.push_back(tab);
      }
    }

    int free_size = client_size - sum_best_size;

    while (!small_tabs.empty()) {
      const int avg_free_size = free_size / static_cast<int>(small_tabs.size());
      free_size %= small_tabs.size();

      if (avg_free_size == 0) {
        // Give 1px to each small tab.
        foreach (Tab* tab, small_tabs) {
          if (free_size == 0) {
            break;
          }
          ++tab->size;
          --free_size;
        }
        break;
      }

      for (TabList::iterator it = small_tabs.begin(); it != small_tabs.end(); ) {
        Tab* tab = *it;
        int needed_size = tab_default_size_ - tab->size;
        if (needed_size > avg_free_size) {
          tab->size += avg_free_size;
          ++it;
        } else {
          // This tab doesn't need that much size.
          tab->size = tab_default_size_;
          // Return extra free size back.
          free_size += avg_free_size - needed_size;
          // This tab is not small any more, remove it from small tab list.
          it = small_tabs.erase(it);
        }
      }
    }

    // Save free size.
    free_size_ = free_size;
    wxLogDebug(wxT("ResizeTabs, free_size: %d"), free_size_);

  } else { // sum_best_size >= client_size
    // Reduce the size of large tabs.
    int lack_size = sum_best_size - client_size;

    for (int large_size = tab_default_size_; large_size > tab_min_size_ && lack_size > 0; large_size /= 2) {
      TabList large_tabs;
      foreach (Tab* tab, tabs_) {
        if (tab->size > large_size) {
          large_tabs.push_back(tab);
        }
      }

      if (large_tabs.empty()) {
        continue;
      }

      while (!large_tabs.empty()) {
        const int avg_lack_size = lack_size / large_tabs.size();
        lack_size %= large_tabs.size();

        if (avg_lack_size == 0) {
          // Take 1px from first "lack_size" number of large tabs.
          foreach (Tab* tab, large_tabs) {
            if (lack_size == 0) {
              break;
            }
            --tab->size;
            --lack_size;
          }
          break;
        }

        for (TabList::iterator it = large_tabs.begin(); it != large_tabs.end(); ) {
          Tab* tab = *it;
          int givable_size = tab->size - large_size;
          if (givable_size > avg_lack_size) {
            tab->size -= avg_lack_size;
            ++it;
          } else {
            // This tab cannot give that much size. Give all it can give.
            tab->size = large_size;
            // Return extra lack size back.
            lack_size += avg_lack_size - givable_size;
            // This tab is not large any more, remove it from large tab list.
            it = large_tabs.erase(it);
          }
        }
      }
    }
  }

  if (refresh) {
    // Refresh if the size of tabs is changed.
    int sum_size = 0;
    foreach (Tab* tab, tabs_) {
      sum_size += tab->size;
    }
    if (sum_size != sum_prev_size) {
      RefreshTabArea();
    }
  }
}

void Notebook::OnTabSize(wxSizeEvent& evt) {
  if (!batch_) {
    ResizeTabs();
  }
  evt.Skip();
}

#ifdef __WXMSW__
#if USE_GDIPLUS_DIRECTLY

static inline Gdiplus::Color GdiplusColor(const wxColour& wx_color) {
  return Gdiplus::Color(wx_color.Alpha(), wx_color.Red(), wx_color.Green(), wx_color.Blue());
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

#endif // USE_GDIPLUS_DIRECTLY
#endif // __WXMSW__

// Paint tab items.
void Notebook::OnTabPaint(wxAutoBufferedPaintDC& dc, wxPaintEvent& evt) {

#ifdef __WXMAC__
  wxGraphicsContext* gc = dc.GetGraphicsContext();
#else
  wxGCDC gcdc(dc);
  wxGraphicsContext* gc = gcdc.GetGraphicsContext();
#endif // __WXMAC__

  DrawBackground(gc);
  DrawForeground(gc, dc);
}

void Notebook::DrawTabBackground(Tab* tab, wxGraphicsContext* gc, int x) {
  const wxRect rect = tab_area_->GetClientRect();

  if (tab->active) {
    gc->SetPen(wxPen(theme_->GetColor(ACTIVE_TAB_BORDER)));
    gc->SetBrush(wxBrush(theme_->GetColor(ACTIVE_TAB_BG)));
  } else {
    gc->SetPen(wxPen(theme_->GetColor(TAB_BORDER)));
    gc->SetBrush(wxBrush(theme_->GetColor(TAB_BG)));
  }

  wxRect tab_rect(x + 1, rect.GetTop() + kMarginTop, tab->size - 1, rect.GetHeight() - kMarginTop + 1);

  wxGraphicsPath border_path = gc->CreatePath();
  border_path.MoveToPoint(tab_rect.GetLeft(), tab_rect.GetBottom());

  const wxDouble kRadius = 4.0f;
  // Top left corner.
  border_path.AddArc(tab_rect.GetLeft() + kRadius, tab_rect.GetTop() + kRadius, kRadius, base::kRadian180, base::kRadian270, true);
  // Top line.
  border_path.AddLineToPoint(tab_rect.GetRight() - kRadius, tab_rect.GetTop());
  // Top right corner.
  border_path.AddArc(tab_rect.GetRight() - kRadius, tab_rect.GetTop() + kRadius, kRadius, base::kRadian270, base::kRadian0, true);
  // Right line.
  border_path.AddLineToPoint(tab_rect.GetRight(), tab_rect.GetBottom());

  gc->DrawPath(border_path);

  if (!tab->active) {
    gc->StrokeLine(x, rect.GetBottom(), x + tab->size + 1, rect.GetBottom());
  }
}

void Notebook::DrawBackground(wxGraphicsContext* gc) {
  const wxRect rect = tab_area_->GetClientRect();

  int x = rect.GetLeft();

  foreach (Tab* tab, tabs_) {
    DrawTabBackground(tab, gc, x);
    x += tab->size;
  }

  if (x <= rect.GetRight()) {
    gc->StrokeLine(x, rect.GetBottom(), rect.GetRight() + 1, rect.GetBottom());
  }
}

void Notebook::DrawForeground(wxGraphicsContext* gc, wxDC& dc) {
  const wxRect rect = tab_area_->GetClientRect();

  gc->SetFont(tab_area_->GetFont(), theme_->GetColor(ACTIVE_TAB_FG));

#ifdef __WXMSW__
#if USE_GDIPLUS_DIRECTLY
  std::auto_ptr<Gdiplus::Graphics> graphics;
  std::auto_ptr<Gdiplus::Font> gdip_font;
#endif // USE_GDIPLUS_DIRECTLY
#endif // __WXMSW__

  int x = rect.GetLeft();

  for (TabList::iterator it = tabs_.begin(); it != tabs_.end(); ++it) {
    Tab* tab = *it;

    const wxRect tab_rect(x + kTabPaddingLeft,
                          rect.GetTop() + kTabPaddingTop + kMarginTop,
                          tab->size - kTabPaddingX,
                          rect.GetHeight() - kTabPaddingTop - kMarginTop);

    if (tab_rect.GetWidth() > 0) {
      if (tab->best_size <= tab->size) {
        // Enough space to display the label completely.
        gc->DrawText(tab->page->NPage_GetLabel(), tab_rect.GetLeft(), tab_rect.GetTop());
      } else {
        // No enough space.
        wxString label = tab->page->NPage_GetLabel();

        // Fit label to the given space.
        label = label.Mid(0, editor::TailorLabel(dc, label, tab_rect.GetWidth()));

        wxDouble fade_x = tab_rect.GetLeft();

        if (label.size() > kFadeAwayChars) {
          wxString label_before = label.Mid(0, label.size() - kFadeAwayChars);
          gc->DrawText(label_before, tab_rect.GetLeft(), tab_rect.GetTop());

          wxDouble label_before_width = 0.0f;
          gc->GetTextExtent(label_before, &label_before_width, NULL, NULL, NULL);
          fade_x += label_before_width;

          label = label.Mid(label.size() - kFadeAwayChars);
        }

        // Fade away the last several characters.

        wxDouble label_width = 0.0f;
        gc->GetTextExtent(label, &label_width, NULL, NULL, NULL);

#ifdef __WXMSW__
#if USE_GDIPLUS_DIRECTLY
        // Lazily create GDI+ objects.
        if (graphics.get() == NULL) {
          graphics.reset(new Gdiplus::Graphics((HDC)dc.GetHDC()));
          gdip_font.reset(GdiplusNewFont(tab_area_->GetFont()));
        }

        const wxColour& tab_bg = tab->active ? theme_->GetColor(ACTIVE_TAB_BG) : theme_->GetColor(TAB_BG);
        const wxColour& tab_fg = tab->active ? theme_->GetColor(ACTIVE_TAB_FG) : theme_->GetColor(TAB_FG);
        std::auto_ptr<Gdiplus::Brush> brush(new Gdiplus::LinearGradientBrush(
          GdiplusRect(wxRect(fade_x, tab_rect.GetTop(), std::ceil(label_width), tab_rect.GetHeight())),
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

#endif // USE_GDIPLUS_DIRECTLY
#else
        // TODO: Fade away.
        gc->DrawText(label, fade_x, tab_rect.GetTop());
#endif // __WXMSW__
      }
    }

    // TODO
    // Modified indicator.
    if (tab->page->NPage_IsModified()) {
      dc.SetTextForeground(*wxRED);
      dc.DrawText(wxT("*"), tab_rect.GetRight() + 5, tab_rect.GetTop());
      // Restore
      dc.SetTextForeground(theme_->GetColor(TAB_FG));
    }

    x += tab->size;
  }
}

void Notebook::OnTabMouse(wxMouseEvent& evt) {
  wxEventType evt_type = evt.GetEventType();

  if (evt_type == wxEVT_LEFT_UP) {
    HandleTabMouseLeftUp(evt);
  } else if (evt_type == wxEVT_MOTION) {
    HandleTabMouseMotion(evt);
  } else if (evt_type == wxEVT_RIGHT_UP) {
    HandleTabMouseRightUp(evt);
  } else if (evt_type == wxEVT_MIDDLE_DOWN) {
    HandleTabMouseMiddleDown(evt);
  } else if (evt_type == wxEVT_MIDDLE_UP) {
    HandleTabMouseMiddleUp(evt);
  } if (evt_type == wxEVT_LEFT_DCLICK) {
    HandleTabMouseLeftDClick(evt);
  }

  evt.Skip();
}

void Notebook::HandleTabMouseLeftUp(wxMouseEvent& evt) {
  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    if (!evt.CmdDown()) {
      ActivatePage(it);
    } else {
      // Close the tab.
      rclicked_tab_ = *it;
      wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_NOTEBOOK_RCLICK_CLOSE);
      GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
    }
  } else {
    if (evt.CmdDown()) {
      // New file.
      wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_NOTEBOOK_RCLICK_NEW_FILE);
      GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
    }
  }
}

void Notebook::HandleTabMouseMiddleDown(wxMouseEvent& evt) {
  if (!tab_area_->HasCapture()) {
    tab_area_->CaptureMouse();
  }
}

void Notebook::HandleTabMouseMiddleUp(wxMouseEvent& evt) {
  if (tab_area_->HasCapture()) {
    tab_area_->ReleaseMouse();
  }

  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    return;
  }

  RemovePage(it);
}

// Update tooltip on mouse motion.
void Notebook::HandleTabMouseMotion(wxMouseEvent& evt) {
  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    tab_area_->SetToolTipEx(wxEmptyString);
    return;
  }

  Tab* tab = *it;
  tab_area_->SetToolTipEx(tab->page->NPage_GetToolTip());
}

void Notebook::HandleTabMouseRightUp(wxMouseEvent& evt) {
  rclicked_tab_ = NULL; // Clear

  wxMenu menu;

  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it != tabs_.end()) {
    rclicked_tab_ = *it;
    TextPage* rclicked_page = rclicked_tab_->page;

    // Add menu items according to the text page clicked.
    // Note: If a menu item shouldn't exist in the context, don't create it.

    if (rclicked_page->buffer_modified()) {
      menu.Append(ID_MENU_NOTEBOOK_RCLICK_SAVE, kTrRClickSave);
    }

    menu.Append(ID_MENU_NOTEBOOK_RCLICK_CLOSE, kTrRClickClose);

    if (PageCount() > 1) {
      menu.Append(ID_MENU_NOTEBOOK_RCLICK_CLOSE_ALL, kTrRClickCloseAll);
      menu.Append(ID_MENU_NOTEBOOK_RCLICK_CLOSE_ALL_BUT_THIS, kTrRClickCloseAllButThis);
    }

    if (!rclicked_page->buffer_new_created()) {
      menu.AppendSeparator();
      menu.Append(ID_MENU_NOTEBOOK_RCLICK_COPY_PATH, kTrRClickCopyFilePath);
      menu.Append(ID_MENU_NOTEBOOK_RCLICK_OPEN_FOLDER, kTrRClickOpenFolder);
    }
  } else { // Click on blank area
    menu.Append(ID_MENU_NOTEBOOK_RCLICK_NEW_FILE, kTrRClickNewFile);
  }

  PopupMenu(&menu, evt.GetPosition());
}

// Left double-click on blank area, new a file.
void Notebook::HandleTabMouseLeftDClick(wxMouseEvent& evt) {
  TabList::iterator it = GetTabByPos(evt.GetPosition().x);
  if (it == tabs_.end()) {
    wxCommandEvent cmd_evt(wxEVT_COMMAND_MENU_SELECTED, ID_MENU_NOTEBOOK_RCLICK_NEW_FILE);
    GetParent()->GetEventHandler()->AddPendingEvent(cmd_evt);
  }
}

void Notebook::OnTextFileNameChange(wxCommandEvent& evt) {
  // Update page tab.
  wxObject* evt_obj = evt.GetEventObject();
  wxWindow* page_window = wxDynamicCast(evt_obj, wxWindow);
  if (page_window == NULL) {
    return;
  }

  Tab* tab = GetTabByWindow(page_window);
  if (tab == NULL) {
    return;
  }

  // Update page info.
  tab->size = CalcTabBestSize(tab->page->NPage_GetLabel());

  RefreshTabArea();
}

void Notebook::OnTextModifiedChange(wxCommandEvent& evt) {
  // Update page tab.
  wxObject* evt_obj = evt.GetEventObject();
  wxWindow* page_window = wxDynamicCast(evt_obj, wxWindow);
  if (page_window == NULL) {
    return;
  }

  Tab* tab = GetTabByWindow(page_window);
  if (tab == NULL) {
    return;
  }

  RefreshTabArea();
}

Notebook::TabList::iterator Notebook::GetTabByPos(int pos_x) {
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

Notebook::Tab* Notebook::GetTabByWindow(wxWindow* window, size_t* index) {
  size_t i = 0;

  foreach (Tab* tab, tabs_) {
    if (tab->page == window) {
      if (index != NULL) {
        *index = i;
      }
      return tab;
    }

    ++i;
  }

  return NULL;
}

void Notebook::ActivatePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return;
  }

  Tab* tab = *it;
  if (tab->active) {
    return; // Already active
  }

  page_area_->Freeze();

  // Deactivate previous active page.
  TabList::iterator active_it = GetActiveTab();
  if (active_it != tabs_.end()) {
    (*active_it)->active = false;
    (*active_it)->page->NPage_Activate(false);
    page_sizer_->Clear(false);
  }

  // Activate new page.
  (*it)->active = true;
  page_sizer_->Add((*it)->page, 1, wxEXPAND);
  (*it)->page->NPage_Activate(true);

  // Update active age list.
  active_age_list_.remove(tab);
  active_age_list_.push_front(tab);

  page_area_->Layout();
  page_area_->Thaw();

  tab_area_->Refresh();

  PostPageChangedEvent();
}

bool Notebook::RemovePage(TabList::iterator it) {
  if (it == tabs_.end()) {
    return false;
  }

  Tab* tab = *it;

  if (!tab->page->NPage_Close()) {
    // Failed to close page, stop removing.
    return false;
  }

  // Reset right-clicked tab since the page will be destroyed.
  if (rclicked_tab_ != NULL && rclicked_tab_->page == tab->page) {
    rclicked_tab_ = NULL;
  }

  page_area_->Freeze();

  tabs_.erase(it);
  active_age_list_.remove(tab);

  // The page to remove is active.
  if (tab->active) {
    tab->page->Hide();
    page_sizer_->Clear(false);
    tab->page->Destroy();

    if (PageCount() > 0) {
      // Activate another page.
      Tab* active_page_info = active_age_list_.front();
      active_page_info->active = true;
      page_sizer_->Add(active_page_info->page, 1, wxEXPAND);
      active_page_info->page->NPage_Activate(true);

      PostPageChangedEvent();
    }
  }

  delete tab;

  page_area_->Layout();
  page_area_->Thaw();

  // The tab is removed, more space is available, resize the left tabs.
  ResizeTabs(/*refresh=*/false);
  tab_area_->Refresh();

  return true;
}

Notebook::TabList::iterator Notebook::GetActiveTab() {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->active) {
      break;
    }
  }
  return it;
}

Notebook::TabList::iterator Notebook::GetTabByPage(const TextPage* page) {
  TabList::iterator it = tabs_.begin();
  for (; it != tabs_.end(); ++it) {
    if ((*it)->page == page) {
      break;
    }
  }
  return it;
}

int Notebook::CalcTabBestSize(const wxString& label) const {
  std::auto_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(tab_area_));
  gc->SetFont(tab_area_->GetFont(), *wxWHITE);

  wxDouble label_size = 0.0f;
  gc->GetTextExtent(label, &label_size, NULL, NULL, NULL);

  return static_cast<int>(std::ceil(label_size)) + kTabPaddingX;
}

void Notebook::RefreshTabArea() {
  tab_area_->Refresh();
}

void Notebook::PostPageChangedEvent() {
  wxCommandEvent evt(kEvtNotebookPageChanged, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

////////////////////////////////////////////////////////////////////////////////
// Events

DEFINE_EVENT_TYPE(kEvtNotebookPageChanged);

} // namespace jil
