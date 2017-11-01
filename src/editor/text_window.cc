#include "editor/text_window.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#if JIL_USE_WX_CARET
#include "wx/caret.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/menu.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/timer.h"

#include "base/string_util.h"
#include "base/math_util.h"

#include "ui/color.h"

#include "editor/action.h"
#include "editor/compile_config.h"
#include "editor/util.h"
#include "editor/style.h"
#include "editor/line_nr_area.h"
#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "editor/wrap.h"
#include "editor/tab.h"
#include "editor/text_extent.h"
#include "editor/renderer.h"
#include "editor/ft_plugin.h"

// When to update virtual size?
// - OnTextSize: Because virtual size depends on client size of text area.
// - OnLineAdded, OnLineDeleted
// - Text font is changed
// - Wrap changes

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(kTextWindowEvent);

////////////////////////////////////////////////////////////////////////////////

enum {
  ID_SCROLL_TIMER = wxID_HIGHEST + 100,

#if !JIL_USE_WX_CARET
  ID_CARET_TIMER,
#endif
};

static const int kMarginTop = 3;
static const int kMarginBottom = 0;
static const int kTextMarginLeft = 0;
static const int kTextMarginBottom = 0;
static const int kLineNrPadding = 15;
static const int kLineNrPaddingRight = 9;
static const int kLineNrHlPaddingRight = 3;
static const int kCaretWidth = 1;

// Displayed on the blank lines of line number area.
// A respect to Vim.
static const wxString kTilde = wxT("~");

//------------------------------------------------------------------------------

IMPLEMENT_CLASS(TextWindow, wxScrolledWindow)

BEGIN_EVENT_TABLE(TextWindow, wxScrolledWindow)
EVT_SIZE      (TextWindow::OnSize)
EVT_TIMER     (ID_SCROLL_TIMER,   TextWindow::OnScrollTimer)
#if !JIL_USE_WX_CARET
EVT_TIMER     (ID_CARET_TIMER,    TextWindow::OnCaretTimer)
#endif
END_EVENT_TABLE()

//------------------------------------------------------------------------------

TextWindow::TextWindow(TextBuffer* buffer)
    : buffer_(buffer) {
  Init();
}

bool TextWindow::Create(wxWindow* parent, wxWindowID id, bool hide) {
  assert(theme_);
  assert(style_ != NULL);
  assert(binding_ != NULL);

  if (hide) {
    // Yes, Hide() can be called before the window is created.
    Hide();
  }

  // Create scrolled window.
  if (!wxScrolledWindow::Create(parent, id)) {
    return false;
  }

  DoShowHScrollbar();

  const wxColour& normal_bg = style_->Get(Style::kNormal)->bg();

  // Use the normal text background for text window itself.
  SetBackgroundColour(normal_bg);

  // Create line number area.
  line_nr_area_ = new LineNrArea(this);
  if (!line_nr_area_->Create(this, wxID_ANY)) {
    return false;
  }
  line_nr_area_->SetBackgroundColour(style_->Get(Style::kNumber)->bg());
  line_nr_area_->SetCursor(wxCursor(wxCURSOR_RIGHT_ARROW));

  // Create text area.
  text_area_ = new TextArea(this);
  if (!text_area_->Create(this, wxID_ANY)) {
    return false;
  }
  text_area_->SetBackgroundColour(normal_bg);
  text_area_->SetCursor(wxCURSOR_IBEAM);

  SetTargetWindow(text_area_);

  // Font, char size, line height, caret size, scrollbar, ... will be set in
  // SetTextFont() which shall be called after Create().

#if JIL_USE_WX_CARET
  // Caret
  // NOTE: Can't set height to -1 under GTK or there will be bitmap create assert failure.
  wxCaret* caret = new wxCaret(text_area_, 1, 1);
  text_area_->SetCaret(caret);
  caret->Show();
#else
  caret_.timer = new wxTimer(this, ID_CARET_TIMER);
  caret_.timer->Start(caret_.interval_ms, false);
#endif  // JIL_USE_WX_CARET

  // Attach buffer listener when text window is actually created.
  buffer_->AttachListener(this);

  return true;
}

TextWindow::~TextWindow() {
  StopScrollTimer();

#if !JIL_USE_WX_CARET
  wxDELETE(caret_.timer);
#endif

  if (buffer_ != NULL) {
    buffer_->DetachListener(this);
  }

  wxDELETE(text_extent_);
}

//------------------------------------------------------------------------------

void TextWindow::ReapplyTheme() {
  assert(theme_);
  assert(style_ != NULL);

  const wxColour& normal_bg = style_->Get(Style::kNormal)->bg();
  SetBackgroundColour(normal_bg);

  line_nr_area_->SetBackgroundColour(style_->Get(Style::kNumber)->bg());
  text_area_->SetBackgroundColour(normal_bg);

  Refresh();
}

bool TextWindow::HasFocus() const {
  if (text_area_ != NULL && text_area_->HasFocus()) {
    return true;
  }

  if (wxScrolledWindow::HasFocus()) {
    return true;
  }

  return false;
}

void TextWindow::SetFocus() {
  if (text_area_ != NULL) {
    text_area_->SetFocus();
  }
}

//------------------------------------------------------------------------------

void TextWindow::SetTextFont(const wxFont& font) {
  text_area_->SetOwnFont(font);
  line_nr_area_->SetOwnFont(font);

  int old_line_nr_width = line_nr_width_;

  text_extent_->SetFont(font);
  char_size_ = text_extent_->char_size();

  UpdateLineHeight();
  HandleLineHeightChange();

  UpdateLineNrWidth();

  // If the line number width changes, layout the areas.
  if (old_line_nr_width != line_nr_width_) {
    LayoutAreas();
  }

  text_area_->Refresh();
  line_nr_area_->Refresh();
}

void TextWindow::SetLinePadding(int line_padding) {
  if (line_padding_ != line_padding) {
    line_padding_ = line_padding;

    UpdateLineHeight();
    HandleLineHeightChange();

    text_area_->Refresh();
    line_nr_area_->Refresh();
  }
}

//------------------------------------------------------------------------------

FtPlugin* TextWindow::ft_plugin() const {
  return buffer_->ft_plugin();
}

//------------------------------------------------------------------------------

wxString TextWindow::buffer_file_name() const {
  return buffer_->file_name();
}

bool TextWindow::buffer_modified() const {
  return buffer_->modified();
}

bool TextWindow::buffer_new_created() const {
  return buffer_->new_created();
}

//------------------------------------------------------------------------------

void TextWindow::OnBufferLineChange(LineChangeType type, const LineChangeData& data) {
  if (type != kLineRefresh) {
    // It's very difficult to update the find result.
    // Just clear it right now.
    ClearFindResult();
  }

  switch (type) {
  case kLineAdded:
    HandleLineAdded(data);
    break;

  case kLineUpdated:
    HandleLineUpdated(data);
    break;

  case kLineDeleted:
    HandleLineDeleted(data);
    break;

  case kLineRefresh:
    RefreshTextByLineRange(data);
    break;

  default:
    break;
  }
}

void TextWindow::OnBufferChange(ChangeType type) {
  switch (type) {
  case kEncodingChange:
    PostEvent(kEncodingEvent);
    break;

  case kFileFormatChange:
    PostEvent(kFileFormatEvent);
    break;

  case kFileNameChange:
    PostEvent(kFileNameEvent);
    break;

  case kModifiedStateChange:
    PostEvent(kModifiedStateEvent);
    break;

  case kFileTypeChange:
    HandleFileTypeChange();
    break;

  case kTabOptionsChange:
    HandleTabOptionsChange();
    break;

  case kViewOptionsChange:
    // Update view and view options using the view options from buffer.
    SetViewOptions(buffer_->view_options());
    break;

  default:
    break;
  }
}

const TextOptions& TextWindow::text_options() const {
  return buffer_->text_options();
}

void TextWindow::Wrap(bool wrap) {
  if (view_options_.wrap != wrap) {
    view_options_.wrap = wrap;
    DoWrap();
  }
}

void TextWindow::ShowNumber(bool show_number) {
  if (view_options_.show_number != show_number) {
    view_options_.show_number = show_number;
    DoShowNumber();
  }
}

void TextWindow::ShowSpace(bool show_space) {
  if (view_options_.show_space != show_space) {
    view_options_.show_space = show_space;
    text_area_->Refresh();
  }
}

void TextWindow::ShowHScrollbar(bool show_hscrollbar) {
  if (view_options_.show_hscrollbar != show_hscrollbar) {
    view_options_.show_hscrollbar = show_hscrollbar;
    DoShowHScrollbar();
  }
}

void TextWindow::SetRulers(const std::vector<int>& rulers) {
  if (view_options_.rulers != rulers) {
    view_options_.rulers = rulers;
    text_area_->Refresh();
  }
}

void TextWindow::SetViewOptions(const ViewOptions& new_view_options) {
  if (view_options_.wrap != new_view_options.wrap) {
    Wrap(new_view_options.wrap);
  }

  if (view_options_.show_number != new_view_options.show_number) {
    ShowNumber(new_view_options.show_number);
  }

  if (view_options_.show_space != new_view_options.show_space) {
    ShowSpace(new_view_options.show_space);
  }

  if (view_options_.show_hscrollbar != new_view_options.show_hscrollbar) {
    ShowHScrollbar(new_view_options.show_hscrollbar);
  }

  if (view_options_.rulers != new_view_options.rulers) {
    SetRulers(new_view_options.rulers);
  }
}

//------------------------------------------------------------------------------


#if JIL_ENABLE_LEADER_KEY
void TextWindow::set_leader_key(Key* key) {
  if (leader_key_ != key) {
    leader_key_ = key;
  }
}
#endif  // JIL_ENABLE_LEADER_KEY

Mode TextWindow::mode() const {
  if (selection_.IsEmpty()) {
    return editor::kNormalMode;
  } else {
    return editor::kVisualMode;
  }
}

void TextWindow::Exec(Action* action) {
  // The returned action might be a new one if it's merged to the previous
  // action, or NULL if it's not effective.
  action = buffer_->AddAction(action);

  if (action != NULL) {
    UpdateAfterExec(action);
  }
}

void TextWindow::SetFileFormat(FileFormat file_format) {
  if (file_format != buffer_->file_format()) {
    SetFileFormatAction* sffa = new SetFileFormatAction(buffer_, caret_point_, file_format);
    sffa->set_caret_point(caret_point_);
    sffa->set_update_caret(false);
    Exec(sffa);
  }
}

bool TextWindow::CanUndo() const {
  return buffer_->CanUndo();
}

bool TextWindow::CanRedo() const {
  return buffer_->CanRedo();
}

void TextWindow::Undo() {
  if (!CanUndo()) {
    return;
  }

  Action* action = buffer_->Undo();
  UpdateAfterUndo(action);

  if (action->grouped()) {
    while (true) {
      Action* action = buffer_->Undo();
      if (action == NULL) {
        break;
      }
      UpdateAfterUndo(action);
      if (action->grouped()) {
        break;
      }
    }
  }
}

void TextWindow::Redo() {
  if (!CanRedo()) {
    return;
  }

  Action* action = buffer_->Redo();
  UpdateAfterExec(action);

  if (action->grouped()) {
    while (true) {
      Action* action = buffer_->Redo();
      if (action == NULL) {
        break;
      }
      UpdateAfterExec(action);
      if (action->grouped()) {
        break;
      }
    }
  }
}

void TextWindow::SelectText(TextUnit text_unit, SeekType seek_type) {
  if (text_unit == kBuffer && seek_type == kWhole) {
    // Select the whole buffer.
    Coord line_count = buffer_->LineCount();
    TextPoint point_begin(0, 1);
    TextPoint point_end(buffer_->LineLength(line_count), line_count);
    SetSelection(point_begin, point_end, false);
    return;
  }

  TextPoint point = buffer_->Seek(caret_point_, text_unit, seek_type);
  if (point != caret_point_) {
    ExtendSelection(point, false);
  }
}

void TextWindow::InsertChar(wchar_t c) {
  bool grouped = false;

  if (!selection_.IsEmpty()) {
    // NOTE: This might change the caret point.
    DeleteSelection(true, true);
    grouped = true;
  }

  TextPoint point = caret_point_;

  // Handle virtual spaces.
  Coord line_length = buffer_->LineLength(caret_point_.y);
  if (caret_point_.x > line_length) {
    if (c == LF) {
      // Break a new line, no spaces will be inserted.
      point.x = line_length;
    } else {
      // Fill virtual spaces before insert the char.
      std::wstring spaces(caret_point_.x - line_length, kSpaceChar);
      TextPoint p(line_length, caret_point_.y);
      InsertString(p, spaces, !grouped, false);

      if (!grouped) {
        grouped = true;
      }
    }
  }

  InsertChar(point, c, kForward, grouped);
}

void TextWindow::NewLineBreak() {
  // NOTE: If there's any text selected, it will be deleted.
  InsertChar(LF);
}

void TextWindow::NewLineBelow() {
  // Don't delete the selected text, just clear the selection.
  ClearSelection();

  // Insert '\n' at the end of current line.
  TextPoint p(buffer_->LineLength(caret_point_.y), caret_point_.y);
  InsertChar(p, LF, kForward, false);
}

void TextWindow::NewLineAbove() {
  // Don't delete the selected text, just clear the selection.
  ClearSelection();

  // Insert '\n' at the beginning of current line.
  InsertChar(TextPoint(0, caret_point_.y), LF, kBackward, false);
}

void TextWindow::InsertString(const std::wstring& str) {
  assert(!str.empty());

  bool grouped = false;

  // Delete the selected text.
  if (!selection_.IsEmpty()) {
    // NOTE: This might change the caret point.
    DeleteSelection(true, true);
    grouped = true;
  }

  // Handle virtual spaces.
  Coord line_length = buffer_->LineLength(caret_point_.y);
  if (caret_point_.x > line_length) {
    std::wstring spaces(caret_point_.x - line_length, kSpaceChar);
    TextPoint point(line_length, caret_point_.y);

    InsertString(point, spaces, !grouped, false);

    if (!grouped) {
      grouped = true;
    }
  }

  InsertString(caret_point_, str, grouped, true);
}

void TextWindow::InsertString(const TextPoint& point,
                              const std::wstring& str,
                              bool grouped,
                              bool update_caret) {
  InsertStringAction* isa = new InsertStringAction(buffer_, point, str);
  isa->set_caret_point(caret_point_);
  isa->set_update_caret(update_caret);
  isa->set_grouped(grouped);
  Exec(isa);
}

void TextWindow::Move(TextUnit text_unit, SeekType seek_type) {
  TextPoint point = caret_point_;
  bool line_step = false;

  if (text_unit == kSelected) {
    if (!selection_.IsEmpty()) {
      if (seek_type == kBegin) {
        point = selection_.begin();
      } else if (seek_type == kEnd) {
        point = selection_.end();
      }
    }
  } else {
    point = buffer_->Seek(caret_point_, text_unit, seek_type);
    if (text_unit == kLine && (seek_type == kPrev || seek_type == kNext)) {
      line_step = true;
    }
  }

  // Clear selection even won't move actually.
  ClearSelection();

  UpdateCaretPoint(point, line_step, true, false);
}

void TextWindow::DeleteText(TextUnit text_unit, SeekType seek_type) {
  Action* action = NULL;

  if (text_unit == kSelected) {
    if (!selection_.IsEmpty() && !selection_.IsRectEmpty()) {
      action = new DeleteRangeAction(buffer_,
                                     selection_.range,
                                     selection_.dir,
                                     selection_.rect,
                                     true);
    }
  } else if (text_unit == kLine && seek_type == kWhole) {
    TextPoint point(0, caret_point_.y);
    if (point.y == buffer_->LineCount()) {  // Last line
      Coord line_length = buffer_->LineLength(point.y);
      if (line_length != 0) {
        TextRange range(point, TextPoint(line_length, point.y));
        action = new DeleteRangeAction(buffer_, range, kForward, false, false);
      }
    } else {
      action = new DeleteAction(buffer_, point, text_unit, seek_type);
    }
  } else if (text_unit == kBuffer) {
    TextRange range = buffer_->range();
    if (!range.IsEmpty()) {
      action = new DeleteRangeAction(buffer_, range, kForward, false, false);
    }
  } else {
    action = new DeleteAction(buffer_, caret_point_, text_unit, seek_type);
  }

  if (action != NULL) {
    action->set_caret_point(caret_point_);
    action->set_update_caret(true);
    Exec(action);
  }
}

void TextWindow::DeleteRange(const TextRange& range,
                             TextDir dir,
                             bool rect,
                             bool grouped,
                             bool selected,
                             bool update_caret) {
  DeleteRangeAction* dra = new DeleteRangeAction(buffer_, range, dir, rect, selected);

  dra->set_caret_point(caret_point_);
  dra->set_update_caret(update_caret);
  dra->set_grouped(grouped);

  Exec(dra);
}

void TextWindow::DeleteSelection(bool grouped, bool update_caret) {
  assert(!selection_.IsEmpty());

  DeleteRangeAction* dra = new DeleteRangeAction(buffer_,
                                                 selection_.range,
                                                 selection_.dir,
                                                 selection_.rect,
                                                 true);
  dra->set_caret_point(caret_point_);
  dra->set_update_caret(update_caret);
  dra->set_grouped(grouped);

  // Reset before execute the action for refreshing properly.
  selection_.Reset();

  Exec(dra);
}

void TextWindow::Replace(const TextRange& range,
                         const std::wstring& replace_str,
                         bool grouped) {
  grouped = grouped && !replace_str.empty();

  DeleteRange(range, kForward, false, grouped, false, false);

  if (!replace_str.empty()) {
    InsertString(range.point_begin(), replace_str, grouped, false);
  }

  if (!selection_.IsEmpty()) {
    if (!selection_.GetLineRange().Intersect(range.GetLineRange()).IsEmpty()) {
      ClearSelection();
    }
  }

  // Update caret point if necessary.
  if (range.HasLine(caret_point_.y)) {
    Coord line_length = buffer_->LineLength(caret_point_.y);
    if (caret_point_.x > line_length) {
      caret_point_.x = line_length;

      // NOTE: Don't have to call UpdateCaretPoint.
      UpdateCaretPosition();

      PostEvent(kCaretEvent);
    }
  }
}

void TextWindow::ScrollText(TextUnit text_unit, SeekType seek_type) {
  switch (text_unit) {
  case kChar:
    // NOTE: Horizontal scroll is not supported.
    break;

  case kLine:
    ScrollLines(seek_type == kPrev ? -1 : 1);
    break;

  case kHalfPage: {
    int lines = GetHalfPageSize();
    ScrollLinesDirectly(seek_type == kPrev ? -lines : lines);
    break;
  }

  case kPage:
    ScrollPages(seek_type == kPrev ? -1 : 1);
    break;

  default:
    break;
  }
}

void TextWindow::ScrollToPoint(const TextPoint& point) {
  if (!point.Valid()) {
    return;
  }

  int view_start_x = 0;
  int view_start_y = 0;
  GetViewStart(&view_start_x, &view_start_y);

  int y = wxDefaultCoord;

  Coord caret_y = point.y;
  if (view_options_.wrap) {
    caret_y = wrap_helper()->WrapLineNr(caret_y) +
              wrap_helper()->WrappedLineCount(caret_y) -
              1;
  }

  wxRect client_rect = text_area_->GetClientRect();

  // - 1 because line nr is 0-based.
  if (caret_y - 1 < view_start_y) {
    y = caret_y - 1;
  } else {
    int unscrolled_bottom = GetUnscrolledY(client_rect.GetBottom());
    // NOTE: Don't use (unscrolled_bottom + line_height_ / 2) / line_height_
    // Otherwise the line might be partially visible.
    int line_end = (unscrolled_bottom) / line_height_;
    if (caret_y > line_end) {
      y = view_start_y + (caret_y - line_end);
    }
  }

  int x = wxDefaultCoord;

  // If wrap is on, no horizontal scroll.
  if (!view_options_.wrap) {
    const int kScrollRateX = 3;  // 3 units per scroll.
    if (point.x <= view_start_x) {
      x = point.x - kScrollRateX;
      if (x < 0) {
        x = 0;
      }
    } else {
      int unscrolled_right = GetUnscrolledX(client_rect.GetRight());
      int char_end = (unscrolled_right + char_size_.x / 2) / char_size_.x;
      if (point.x >= char_end) {
        // 3 units per scroll.
        x = view_start_x + (point.x - char_end) + kScrollRateX;
      }
    }
  }

  if (x != wxDefaultCoord || y != wxDefaultCoord) {
    Scroll(x, y);
  }
}

void TextWindow::ScrollLinesDirectly(int lines) {
  int view_start_x = 0;
  int view_start_y = 0;
  GetViewStart(&view_start_x, &view_start_y);

  int y = view_start_y + lines;
  Scroll(-1, y);
}

void TextWindow::Goto(Coord ln) {
  // NOTE: Don't use assert().
  if (ln < 1) {
    ln = 1;
  } else if (ln > buffer_->LineCount()) {
    ln = buffer_->LineCount();
  }

  Coord y = ln;

  if (view_options_.wrap) {
    ln = wrap_helper()->WrapLineNr(ln);
  }

  LineRange line_range = GetClientLineRange();

  if (ln < line_range.first()) {
    ScrollLinesDirectly(ln - line_range.first() - GetHalfPageSize());
  } else if (ln > line_range.last()) {
    ScrollLinesDirectly(ln - line_range.last() + GetHalfPageSize());
  }

  Coord x = std::min(max_caret_x_, buffer_->LineLength(y));

  UpdateCaretPoint(TextPoint(x, y), true, false, false);
}

//------------------------------------------------------------------------------

Coord TextWindow::GetPageSize() const {
  return (text_area_->GetClientSize().GetHeight() / line_height_);
}

//------------------------------------------------------------------------------
// Indent

void TextWindow::AutoIndent(Coord ln) {
  AutoIndentLineAction* aila = new AutoIndentLineAction(buffer_, ln);
  aila->set_caret_point(caret_point_);
  aila->set_update_caret(true);
  Exec(aila);
}

void TextWindow::Retab() {
  RetabAction* retab_action = new RetabAction(buffer_);
  retab_action->set_caret_point(caret_point_);
  retab_action->set_update_caret(true);
  Exec(retab_action);
}

//------------------------------------------------------------------------------

void TextWindow::UpdateCaretPoint(const TextPoint& point, bool line_step, bool scroll, bool vspace) {
  TextPoint p(point);

  // Adjust the new caret point.
  Coord line_length = buffer_->LineLength(p.y);
  if (line_step) {
    // The caret point is changed by moving to previous/next line or GoTo(ln).
    p.x = std::min(max_caret_x_, line_length);
  } else {
    if (!vspace) {  // Virtual space not allowed.
      if (p.x > line_length) {
        p.x = line_length;
      }
    }
  }

  if (p == caret_point_) {
    if (scroll) {
      ScrollToPoint(caret_point_);
    }
    return;
  }

  int old_caret_y = caret_point_.y;
  caret_point_ = p;

  if (!line_step) {
    max_caret_x_ = p.x;
  }

  if (scroll) {
    ScrollToPoint(caret_point_);
  }

  // NOTE:
  // If you update caret position before scroll, the position will be wrong.
  UpdateCaretPosition();

#if !JIL_USE_WX_CARET
  // Restart the caret blinking timer so that the caret could be painted
  // immediately after the caret change.
  RestartCaretTimer();
#endif

  // Refresh the highlight of the caret line.
  if (caret_point_.y != old_caret_y) {
    RefreshLineNrByLine(old_caret_y);  // Erase
    RefreshLineNrByLine(caret_point_.y);

#if !JIL_USE_WX_CARET
    // Refresh the caret.
    RefreshTextByLine(old_caret_y);  // Erase
    RefreshTextByLine(caret_point_.y);
#endif
  } else {
#if !JIL_USE_WX_CARET
    // Refresh the caret.
    RefreshTextByLine(caret_point_.y);
#endif
  }

  // Notify the caret change.
  PostEvent(kCaretEvent);
}

//------------------------------------------------------------------------------
// Selection

void TextWindow::SetSelection(const TextRange& range, TextDir dir, bool rect) {
  if (range.IsEmpty()) {
    ClearSelection();
    return;
  }

  Selection old_selection = selection_;
  selection_.Set(range, dir, rect);

  if (old_selection.IsEmpty()) {
    RefreshTextByLineRange(selection_.GetLineRange(), true);
    return;
  }

  LineRange old_line_range = old_selection.GetLineRange();
  LineRange new_line_range = selection_.GetLineRange();

  // When two line ranges are not intersected, just refresh them one by one.
  LineRange intersected = old_line_range.Intersect(new_line_range);
  if (intersected.IsEmpty()) {
    RefreshTextByLineRange(old_line_range, true);
    RefreshTextByLineRange(new_line_range, true);
    return;
  }

  // When two line ranges are intersected, and the selection is by rect, ...
  if (selection_.rect) {
    if (selection_.GetCharRange() != old_selection.GetCharRange()) {
      RefreshTextByLineRange(old_line_range.Union(new_line_range), true);
      return;
    }
  }

  // When two line ranges are intersected, the following implementation is
  // more efficient, though more complicated.
  LineRange old_extra_lines1;
  LineRange old_extra_lines2;
  old_line_range.Substract(intersected, &old_extra_lines1, &old_extra_lines2);
  if (!old_extra_lines1.IsEmpty()) {
    RefreshTextByLineRange(old_extra_lines1, true);
  }
  if (!old_extra_lines2.IsEmpty()) {
    RefreshTextByLineRange(old_extra_lines2, true);
  }

  LineRange new_extra_lines1;
  LineRange new_extra_lines2;
  new_line_range.Substract(intersected, &new_extra_lines1, &new_extra_lines2);
  if (!new_extra_lines1.IsEmpty()) {
    RefreshTextByLineRange(new_extra_lines1, true);
  }
  if (!new_extra_lines2.IsEmpty()) {
    RefreshTextByLineRange(new_extra_lines2, true);
  }

  // If the char range of the first/last line of the intersected lines
  // has been changed, refresh that line.
  Coord ln = intersected.first();
  if (old_selection.GetCharRange(ln) != selection_.GetCharRange(ln)) {
    RefreshTextByLine(ln, true);
  }

  if (intersected.LineCount() > 1) {
    ln = intersected.last();
    if (old_selection.GetCharRange(ln) != selection_.GetCharRange(ln)) {
      RefreshTextByLine(ln, true);
    }
  }
}

void TextWindow::ClearSelection(bool refresh) {
  if (selection_.IsEmpty()) {
    return;
  }

  TextRange old_range = selection_.range;
  selection_.Reset();

  if (refresh) {
    RefreshTextByLineRange(old_range.GetLineRange());
  }
}

//------------------------------------------------------------------------------

void TextWindow::SetFindResult(const TextRange& find_result, bool inc_find) {
  inc_find_ = inc_find;

  if (find_result_ == find_result) {
    return;
  }

  Coord old_find_ln = kInvCoord;
  if (!find_result_.IsEmpty()) {
    old_find_ln = find_result_.line_first();
  }

  find_result_ = find_result;

  Coord find_ln = kInvCoord;
  if (!find_result_.IsEmpty()) {
    find_ln = find_result_.line_first();
  }

  if (old_find_ln != find_ln && old_find_ln != kInvCoord) {
    RefreshTextByLine(old_find_ln);
  }
  if (find_ln != kInvCoord) {
    RefreshTextByLine(find_ln);
  }
}

void TextWindow::ClearFindResult() {
  if (!find_result_.IsEmpty()) {
    SetFindResult(TextRange(), inc_find_);
  }
}

//------------------------------------------------------------------------------

void TextWindow::Init() {
  allow_text_change_ = true;

  view_options_ = buffer_->view_options();

  style_ = NULL;
  binding_ = NULL;

  line_padding_ = 1;
  line_height_ = 0;

  line_nr_width_ = 0;

  caret_point_.Set(0, 1);
  max_caret_x_ = 0;

#if !JIL_USE_WX_CARET
  caret_.pen = wxPen(*wxLIGHT_GREY, 2);
  caret_.timer = NULL;
  caret_.interval_ms = 600;
  caret_.show = false;
#endif

  down_modifiers_ = 0;

  dragged_ = false;

  scroll_timer_ = NULL;
  scroll_dir_ = kScrollDown;

  // Cache some constantly used text options.
  tab_stop_ = buffer_->text_options().tab_stop;
  expand_tab_ = buffer_->text_options().expand_tab;

  text_extent_ = new TextExtent;

  inc_find_ = false;
}

int TextWindow::GetTextClientWidth() const {
 return text_area_->GetClientSize().GetWidth();
}

//------------------------------------------------------------------------------
// Wrap

WrapHelper* TextWindow::wrap_helper() const {
  assert(view_options_.wrap);
  if (!wrap_helper_) {
    wrap_helper_.reset(new WrapHelper(buffer_, text_extent_));
    wrap_helper_->set_client_width(GetTextClientWidth());
  }
  return wrap_helper_.get();
}

void TextWindow::DoWrap() {
  int wrap_delta = 0;
  bool wrap_changed = false;

  if (view_options_.wrap) {
    wrap_changed = wrap_helper()->Wrap(&wrap_delta);
  } else {
    if (wrap_helper_) {
      wrap_changed = wrap_helper_->Unwrap(&wrap_delta);
      wrap_helper_.reset();
    }
  }

  if (wrap_changed) {
    UpdateTextSize();

    // Virtual height might change due to the wrap change.
    // The wrap-on virtual width is also different from the wrap-off one.
    UpdateVirtualSize();

    // Caret position might change due to the wrap change.
    UpdateCaretPosition();

    text_area_->Refresh();

    if (wrap_delta != 0) {
      line_nr_area_->Refresh();
    }

    // TODO
    //ScrollToPoint(caret_point_);
  }
}

void TextWindow::DoShowHScrollbar() {
  if (view_options_.show_hscrollbar) {
    ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_ALWAYS);
  } else {
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
  }
}

void TextWindow::DoShowNumber() {
  UpdateLineNrWidth();
  LayoutAreas();
  line_nr_area_->Refresh();
}

//------------------------------------------------------------------------------
// Handlers for buffer line changes.

void TextWindow::HandleLineUpdated(const LineChangeData& data) {
  if (!view_options_.wrap) {
    // Update text size and virtual size of areas, etc.
    // NOTE: Virtual height is actually not changed.
    HandleTextChange();

    RefreshTextByLineRange(data, true);
  } else {
    int wrap_delta = 0;
    for (Coord ln = data.first(); ln <= data.last(); ++ln) {
      wrap_delta += wrap_helper()->UpdateLineWrap(ln);
    }

    if (wrap_delta == 0) {
      // NO wrap change. Just refresh the lines in this range.
      RefreshTextByLineRange(data, true);
    } else {
      // Update text size and virtual size of areas, etc.
      HandleTextChange();

      RefreshTextAfterLine(data.first(), false);
      RefreshLineNrAfterLine(data.first(), false);
    }
  }
}

void TextWindow::HandleLineAdded(const LineChangeData& data) {
  if (!view_options_.wrap) {
    // Update text size and virtual size of areas, etc.
    /*bool area_resized = */HandleTextChange();

    // Note (2013-04-20):
    // HandleTextChange() might call LayoutAreas() which will trigger
    // paint event by SetSize(). I really want to avoid the refresh here
    // if LayoutAreas() was called. But the paint event triggered by
    // SetSize() cannot draw the new added line(s) and the lines after it
    // correctly (Tested in WinXP). So always refresh here.

    // if (!area_resized) {
    RefreshTextAfterLine(data.first(), true);

    // Refresh the last "data.LineCount()" lines.
    // There'll be no paint event if they are out of the client rect.
    LineRange ln_refresh_range(buffer_->LineCount() - data.LineCount() + 1,
                               buffer_->LineCount());
    RefreshLineNrByLineRange(ln_refresh_range, true);
    // }
  } else {  // Wrapped
    for (Coord ln = data.first(); ln <= data.last(); ++ln) {
      wrap_helper()->AddLineWrap(ln);
    }

    // Update text size and virtual size of areas, etc.
    HandleTextChange();

    // LineRange wrapped_line_range = wrap_helper()->Wrap(data);
    RefreshTextAfterLine(data.first(), false);

    // LineRange client_line_range = GetClientLineRange();
    // LineRange refresh_line_range(data.first(), client_line_range.last());
    // RefreshLineNrByLineRange(refresh_line_range);
    RefreshLineNrAfterLine(data.first(), false);
  }
}

void TextWindow::HandleLineDeleted(const LineChangeData& data) {
  if (view_options_.wrap) {
    // NOTE: In reverse!
    for (Coord ln = data.last(); ln >= data.first(); --ln) {
      wrap_helper()->RemoveLineWrap(ln);
    }
  }

  // Update text size and virtual size of areas, etc.
  HandleTextChange();

  // Refresh text area.
  if (data.first() == 1) {  // First line is deleted.
    // Refresh the whole client.
    text_area_->Refresh();
  } else if (data.last() >= buffer_->LineCount()) {  // Last line is deleted.
    // Now data.first() is invalid; data.first() - 1 must be valid.
    RefreshTextAfterLine(data.first() - 1, false);
  } else {  // Middle lines are deleted.
    // Now data.first() is still valid.
    RefreshTextAfterLine(data.first(), true);
  }

  // Refresh line nr area.
  if (!view_options_.wrap) {
    RefreshLineNrAfterLine(buffer_->LineCount(), true);
  } else {
    // For simplicity, just call Refresh().
    line_nr_area_->Refresh();
  }
}

//------------------------------------------------------------------------------
// Handlers for buffer changes.

// TODO: tab stop changes need update virtual size, caret pos, etc.
void TextWindow::HandleFileTypeChange() {
  // Update cached text options.
  tab_stop_ = buffer_->text_options().tab_stop;
  expand_tab_ = buffer_->text_options().expand_tab;

  // Update view options.
  ViewOptions old_view_options = view_options_;
  view_options_ = buffer_->view_options();

  // Update display.

  Freeze();

  if (view_options_.wrap != old_view_options.wrap) {
    DoWrap();
  }

  if (view_options_.show_number != old_view_options.show_number) {
    DoShowNumber();
  }

  if (view_options_.show_space != old_view_options.show_space ||
      view_options_.rulers != old_view_options.rulers) {
    text_area_->Refresh();
  }

  if (view_options_.show_hscrollbar != old_view_options.show_hscrollbar) {
    DoShowHScrollbar();
  }

  Thaw();

  PostEvent(kFileTypeEvent);
}

void TextWindow::HandleTabOptionsChange() {
  if (tab_stop_ != buffer_->text_options().tab_stop) {
    tab_stop_ = buffer_->text_options().tab_stop;

    if (view_options_.wrap) {
      DoWrap();
    } else {
      UpdateTextSize();
      UpdateVirtualSize();
    }

    UpdateCaretPosition();

    text_area_->Refresh();
  }

  expand_tab_ = buffer_->text_options().expand_tab;

  PostEvent(kTabOptionsEvent);
}

//------------------------------------------------------------------------------
// System event handlers.

void TextWindow::OnSize(wxSizeEvent& evt) {
  if (m_targetWindow != this) {
    LayoutAreas();
    // NOTE (2013/04/14): Don't have to update virtual size!
  }
  evt.Skip();
}

//------------------------------------------------------------------------------
// Action

void TextWindow::UpdateAfterExec(Action* action) {
  RangeAction* ra = action->AsRangeAction();
  if (ra != NULL) {
    TextRange range = ra->SelectionAfterExec();
    if (!range.IsEmpty()) {
      SetSelection(range, ra->dir(), ra->rect());
    } else {
      ClearSelection();
    }
  }

  if (action->update_caret()) {
    UpdateCaretPointAfterAction(action->CaretPointAfterExec(), ra);
  }
}

void TextWindow::UpdateAfterUndo(Action* action) {
  RangeAction* ra = action->AsRangeAction();
  if (ra != NULL && ra->selected()) {
    // Restore the selection.
    SetSelection(ra->range(), ra->dir(), ra->rect());
  } else {
    ClearSelection();
  }

  if (action->update_caret()) {
    UpdateCaretPointAfterAction(action->caret_point(), ra);
  }
}

void TextWindow::UpdateCaretPointAfterAction(const TextPoint& point, RangeAction* ra) {
  bool vspace = false;

  if (ra != NULL && ra->rect()) {
    // Allow virtual spaces for rect range action.
    vspace = true;
  }

  UpdateCaretPoint(point, false, true, vspace);
}

void TextWindow::InsertChar(const TextPoint& point,
                            wchar_t c,
                            TextDir dir,
                            bool grouped) {
  InsertCharAction* ica = new InsertCharAction(buffer_, point, c);
  ica->set_caret_point(caret_point_);
  ica->set_dir(dir);
  ica->set_grouped(grouped);

  buffer_->AddInsertCharAction(ica);

  UpdateCaretPoint(ica->CaretPointAfterExec(), false, true, false);

  // Indent the new line.
  if (c == LF) {
    TextPoint p = caret_point_;
    if (buffer_->Line(p.y)->IsEmpty(true)) {
      p.x = buffer_->GetExpectedIndent(p.y);
    } else {
      AutoIndent(p.y);
      p.x = buffer_->GetIndentStrLength(p.y);
    }
    UpdateCaretPoint(p, false, true, true);
    return;
  }

  // Re-indent current line if necessary.
  const TextLine* line = buffer_->Line(caret_point_.y);

  // Check if the word (or words, a indent key could be multiple words, e.g.,
  // "End If" in VB) before the caret is an indent key or not.
  if (!line->IsEmpty(true)) {
    Coord off = line->FindNonSpace();
    if (off < caret_point_.x) {
      Coord len = caret_point_.x - off;
      if (ft_plugin()->MatchIndentKey(line->data(), off, len)) {
        AutoIndent(caret_point_.y);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Delegated event handlers from TextArea.

// NOTE: Cannot call Refresh() inside size event handler!
// Size change triggers refresh automatically when it's necessary (when the
// window gets larger - new client area has to be painted). If the window
// gets smaller, there'll be no refresh.
void TextWindow::OnTextSize(wxSizeEvent& evt) {
  wxSize client_size = text_area_->GetClientSize();

  if (client_size.x < kUnreadyWindowSize) {
    return;
  }

  if (!view_options_.wrap) {
    UpdateVirtualSize();
    // Don't call Refresh() here!
  } else {
    // Update wrap information only when client width is changed.
    if (client_size.x != wrap_helper()->client_width()) {
      wrap_helper()->set_client_width(client_size.x);

      int wrap_delta = 0;
      bool wrap_changed = wrap_helper()->Wrap(&wrap_delta);

      if (wrap_changed) {
        UpdateTextSize();
      }

      UpdateVirtualSize();

      if (wrap_changed) {
        if (wrap_delta != 0) {
          line_nr_area_->Refresh();
        }

        // Caret position might change due to the wrap change.
        UpdateCaretPosition();

        // The text has to be repainted since the wrap changes.
        text_area_->Refresh();
      }
    }
  }
}

bool TextWindow::OnTextMouse(wxMouseEvent& evt) {
  bool handled = false;

  wxEventType evt_type = evt.GetEventType();

  if (evt_type == wxEVT_LEFT_DOWN) {
    HandleTextLeftDown(evt);
    handled = true;
  } else if (evt_type == wxEVT_LEFT_UP) {
    HandleTextLeftUp(evt);
    handled = true;
  } else if (evt_type == wxEVT_RIGHT_DOWN) {
    HandleTextRightDown(evt);
    handled = true;
  } else if (evt_type == wxEVT_RIGHT_UP) {
    HandleTextRightUp(evt);
    handled = true;
  } else if (evt_type == wxEVT_MOTION) {
    HandleTextMotion(evt);
    handled = true;
  } else if (evt_type == wxEVT_LEFT_DCLICK) {
    HandleTextLeftDClick(evt);
    handled = true;
  } else if (evt_type == wxEVT_MOUSEWHEEL) {
    handled = HandleTextMouseWheel(evt);
  }

  return handled;
}

// Paint the text area.
void TextWindow::OnTextPaint(Renderer& renderer) {
  if (!view_options_.wrap) {
    HandleTextPaint(renderer);
  } else {
    HandleWrappedTextPaint(renderer);
  }
}

void TextWindow::HandleTextPaint(Renderer& renderer) {
  assert(!view_options_.wrap);

  // Get the lines to paint.
  wxRect update_rect = text_area_->GetUpdateClientRect();
  LineRange update_line_range = LineRangeFromClientRect(update_rect);
  if (update_line_range.IsEmpty()) {
    return;  // No lines to paint.
  }

#if 1
  wxLogDebug("Paint text lines: %d - %d",
             update_line_range.first(),
             update_line_range.last());
#endif

  const wxRect client_rect = text_area_->GetClientRect();
  int x = client_rect.GetLeft();

  // NOTE: Text background is not supported.
  renderer.SetTextBackground(wxNullColour);

  const Coord line_count = buffer_->LineCount();

  int y1 = client_rect.y + line_height_ * (update_line_range.first() - 1);
  int y2 = y1;  // Don't change y1 , it will be used to draw rulers.

  Coord ln = update_line_range.first();
  for (; ln <= update_line_range.last() && ln <= line_count; ++ln) {
    DrawTextLine(renderer, ln, x, y2);
  }

  // Blank lines
  if (ln <= update_line_range.last()) {
    const wxColour& blank_bg = style_->Get(Style::kBlank)->bg();
    int blank_h = line_height_ * (update_line_range.last() - ln + 1);

    if (blank_bg != style_->Get(Style::kNormal)->bg()) {
      renderer.SetStyle(blank_bg, blank_bg, true);

      int blank_w = text_area_->GetVirtualSize().x;
      renderer.DrawRectangle(x, y2, blank_w, blank_h);

      renderer.RestoreStyle();
    }

    y2 += blank_h;
  }

  // Rulers
  if (!view_options_.rulers.empty()) {
    renderer.SetPen(wxPen(theme_->GetColor(COLOR_RULER)), true);

    for (size_t i = 0; i < view_options_.rulers.size(); ++i) {
      int ruler_x = x + char_size_.x * view_options_.rulers[i];
      renderer.DrawLineV(ruler_x, y1, y2);
    }

    renderer.RestorePen();
  }

#if !JIL_USE_WX_CARET
  DrawCaret(renderer, update_line_range);
#endif
}

void TextWindow::HandleWrappedTextPaint(Renderer& renderer) {
  assert(view_options_.wrap);

  // Get the lines to paint.
  wxRect update_rect = text_area_->GetUpdateClientRect();
  LineRange wrapped_update_line_range = LineRangeFromClientRect(update_rect);
  if (wrapped_update_line_range.IsEmpty()) {
    return;  // No lines to paint.
  }

#if 1
  wxLogDebug("Paint wrapped text lines: %d - %d",
             wrapped_update_line_range.first(),
             wrapped_update_line_range.last());
#endif

  const Coord wrapped_line_count = wrap_helper()->WrappedLineCount();

  LineRange blank_line_range;

  if (wrapped_update_line_range.first() > wrapped_line_count) {
    blank_line_range = wrapped_update_line_range;
    wrapped_update_line_range.Set(0);  // No text to draw.
  } else {
    if (wrapped_update_line_range.last() > wrapped_line_count) {
      blank_line_range.Set(wrapped_line_count + 1, wrapped_update_line_range.last());
      // Remove the blank lines.
      wrapped_update_line_range.set_last(wrapped_line_count);
    }
  }

  // Unwrap the wrapped line range.
  LineRange update_line_range;
  if (!wrapped_update_line_range.IsEmpty()) {
    update_line_range = wrap_helper()->UnwrapLineRange(wrapped_update_line_range);
  }

  // NOTE: Text background is not supported.
  renderer.SetTextBackground(wxNullColour);

  const wxRect client_rect = text_area_->GetClientRect();
  int x = client_rect.GetLeft();

  if (!update_line_range.IsEmpty()) {
    Coord wrapped_first_ln = wrap_helper()->WrapLineNr(update_line_range.first());
    int y1 = client_rect.y + line_height_ * (wrapped_first_ln - 1);
    int y2 = y1;  // Don't change y1 , it will be used to draw rulers.

    const Coord line_count = buffer_->LineCount();

    Coord ln = update_line_range.first();
    for (; ln <= update_line_range.last() && ln <= line_count; ++ln) {
      DrawWrappedTextLine(renderer, ln, x, y2);
    }

    // Rulers.
    if (!view_options_.rulers.empty()) {
      renderer.SetPen(wxPen(theme_->GetColor(COLOR_RULER)), true);

      for (size_t i = 0; i < view_options_.rulers.size(); ++i) {
        int ruler_x = x + char_size_.x * view_options_.rulers[i];
        renderer.DrawLineV(ruler_x, y1, y2);
      }

      renderer.RestorePen();
    }
  }

  // Blank lines.
  if (!blank_line_range.IsEmpty()) {
    const wxColour& blank_bg = style_->Get(Style::kBlank)->bg();

    int y = client_rect.y + line_height_ * (blank_line_range.first() - 1);
    int h = line_height_ * blank_line_range.LineCount();

    if (blank_bg != style_->Get(Style::kNormal)->bg()) {
      renderer.SetStyle(blank_bg, blank_bg, true);

      int w = text_area_->GetVirtualSize().x;
      renderer.DrawRectangle(x, y, w, h);

      renderer.RestoreStyle();
    }

    // Rulers.
    if (!view_options_.rulers.empty()) {
      renderer.SetPen(wxPen(theme_->GetColor(COLOR_RULER)), true);

      int y2 = y + h;
      for (size_t i = 0; i < view_options_.rulers.size(); ++i) {
        int ruler_x = x + char_size_.x * view_options_.rulers[i];
        renderer.DrawLineV(ruler_x, y, y2);
      }

      renderer.RestorePen();
    }
  }

#if !JIL_USE_WX_CARET
  DrawCaret(renderer, update_line_range);
#endif
}

#if !JIL_USE_WX_CARET

void TextWindow::DrawCaret(Renderer& renderer, const LineRange& update_line_range) {
  if (caret_.show && update_line_range.Has(caret_point_.y)) {
    renderer.SetPen(caret_.pen, false);

    int height = line_height_ - 1;  // TODO: Verify -1 on Mac and GTK.
    renderer.DrawLineV(caret_.pos.x, caret_.pos.y, caret_.pos.y + height);
  }
}

#endif  // !JIL_USE_WX_CARET

void TextWindow::DrawTextLine(Renderer& renderer, Coord ln, int x, int& y) {
  assert(!view_options_.wrap);

  // If in select range, draw the select background.
  if (selection_.HasLine(ln)) {
    DrawTextLineSelection(renderer, ln, x, y);
  }

  TextLine* line = buffer_->Line(ln);

  int line_text_y = y + line_padding_;

  // Highlight the find matching result.
  if (find_result_.HasLine(ln)) {
    CharRange part2 = find_result_.GetCharRange(ln);
    CharRange part1(0, part2.begin());
    CharRange part3(part2.end(), buffer_->LineLength(ln));

    // For calculating spaces occupied by a tab.
    Coord chars = 0;

    DrawTextLinePart(renderer, line, part1, x, line_text_y, chars);
    DrawTextLineMatchingPart(renderer, line, part2, x, line_text_y, chars);
    DrawTextLinePart(renderer, line, part3, x, line_text_y, chars);
  } else {
    DrawTextLine(renderer, line, x, line_text_y);
  }

  y += line_height_;
}

void TextWindow::DrawTextLineSelection(Renderer& renderer, Coord ln, int x, int y) {
  assert(!view_options_.wrap);
  assert(selection_.HasLine(ln));

  TextLine* line = buffer_->Line(ln);
  CharRange line_selection = selection_.GetCharRange(ln);

  const wxColour& bg = style_->Get(Style::kVisual)->bg();

  if (line_selection.IsEmpty()) {
    if (selection_.rect) {
      renderer.SetStyle(bg, bg, true);

      // Draw a vertical line for empty rect selection.
      int _x = GetLineWidth(line, 0, line_selection.begin());
      renderer.DrawLineV(_x, y, y + line_height_);

      renderer.RestoreStyle();
    }
  } else {
    renderer.SetStyle(bg, bg, true);

    int _x = GetLineWidth(line, 0, line_selection.begin());

    int _w = GetLineWidth(line, line_selection.begin(), line_selection.end());
    if (ln != selection_.end().y && line_selection.end() == kInvCoord) {
      _w += char_size_.x;  // Extra char width for EOL.
    }

    renderer.DrawRectangle(_x, y, _w, line_height_);

    renderer.RestoreStyle();
  }
}

void TextWindow::DrawWrappedTextLine(Renderer& renderer, Coord ln, int x, int& y) {
  assert(view_options_.wrap);

  // If in select range, draw the select background.
  if (selection_.HasLine(ln)) {
    DrawWrappedTextLineSelection(renderer, ln, x, y);
  }

  if (ft_plugin()->IsLexAvailable()) {
    DrawWrappedTextLineWithLex(renderer, ln, x, y);
  } else {
    DrawWrappedTextLineWithoutLex(renderer, ln, x, y);
  }
}

void TextWindow::DrawWrappedTextLineSelection(Renderer& renderer, Coord ln, int x, int y) {
  assert(view_options_.wrap);
  assert(selection_.HasLine(ln));

  const TextLine* line = buffer_->Line(ln);
  CharRange line_selection;

  std::vector<CharRange> sub_ranges = wrap_helper()->SubRanges(ln);

  const wxColour& bg = style_->Get(Style::kVisual)->bg();
  renderer.SetStyle(bg, bg, true);

  line_selection = selection_.GetCharRange(ln);
  int _y = y;

  std::vector<CharRange>::iterator range_it = sub_ranges.begin();
  for (; range_it != sub_ranges.end(); ++range_it) {
    CharRange& sub_range = *range_it;

    CharRange sub_select_char_range = sub_range.Intersect(line_selection);
    if (sub_select_char_range.IsEmpty()) {
      _y += line_height_;
      continue;  // No intersection with the select range.
    }

    int x1 = GetLineWidth(line, sub_range.begin(), sub_select_char_range.begin());
    int x2 = GetLineWidth(line, sub_range.begin(), sub_select_char_range.end());
    int w = x2 - x1;
    if (ln != selection_.end().y && sub_select_char_range.end() == kInvCoord) {
      w += char_size_.x;  // Extra char width for EOL.
    }

    renderer.DrawRectangle(x1, _y, w, line_height_);

    _y += line_height_;
  }

  renderer.RestoreStyle();
}

void TextWindow::DrawWrappedTextLineWithLex(Renderer& renderer, Coord ln, int x, int& y) {
  assert(view_options_.wrap);
  assert(ft_plugin()->IsLexAvailable());

  const TextLine* line = buffer_->Line(ln);

  // For calculating spaces occupied by a tab.
  Coord chars = 0;

  CharRange fr_char_range;
  if (find_result_.HasLine(ln)) {
    fr_char_range = find_result_.GetCharRange(ln);
  }

  std::vector<CharRange> sub_ranges = wrap_helper()->SubRanges(ln);

  for (CharRange& sub_range : sub_ranges) {
    if (sub_range.end() == kInvCoord) {  // Last sub range.
      sub_range.set_end(line->Length());
    }

    int _x = x;
    int text_y = y + line_padding_;

    if (fr_char_range.IsEmpty()) {
      DrawTextLinePart(renderer, line, sub_range, _x, text_y, chars);
    } else {
      // Highlight the find matching result.
      CharRange part2 = fr_char_range.Intersect(sub_range);
      if (part2.IsEmpty()) {
        DrawTextLinePart(renderer, line, sub_range, _x, text_y, chars);
      } else {
        CharRange part1(sub_range.begin(), part2.begin());
        CharRange part3(part2.end(), sub_range.end());

        DrawTextLinePart(renderer, line, part1, _x, text_y, chars);
        DrawTextLineMatchingPart(renderer, line, part2, _x, text_y, chars);
        DrawTextLinePart(renderer, line, part3, _x, text_y, chars);
      }
    }

    y += line_height_;
  }
}

void TextWindow::DrawWrappedTextLineWithoutLex(Renderer& renderer, Coord ln, int x, int& y) {
  assert(view_options_.wrap);
  assert(!ft_plugin()->IsLexAvailable());

  const TextLine* line = buffer_->Line(ln);

  // For calculating spaces occupied by a tab.
  Coord chars = 0;

  CharRange fr_char_range;
  if (find_result_.HasLine(ln)) {
    fr_char_range = find_result_.GetCharRange(ln);
  }

  std::vector<CharRange> sub_ranges = wrap_helper()->SubRanges(ln);

  for (CharRange& sub_range : sub_ranges) {
    if (sub_range.end() == kInvCoord) {  // Last sub range
      sub_range.set_end(line->Length());
    }

    int _x = x;
    int text_y = y + line_padding_;

    if (fr_char_range.IsEmpty()) {
      DrawTextLinePart(renderer, line, sub_range, _x, text_y, chars);
    } else {
      // Highlight the find matching result.
      CharRange part2 = fr_char_range.Intersect(sub_range);
      if (part2.IsEmpty()) {
        DrawTextLinePiece(renderer, line->data(), sub_range.begin(), sub_range.end(), Lex(), _x, text_y, chars);
      } else {
        CharRange part1(sub_range.begin(), part2.begin());
        CharRange part3(part2.end(), sub_range.end());

        DrawTextLinePiece(renderer, line->data(), part1.begin(), part1.end(), Lex(), _x, text_y, chars);
        DrawTextLineMatchingPart(renderer, line, part2, _x, text_y, chars);
        DrawTextLinePiece(renderer, line->data(), part3.begin(), part3.end(), Lex(), _x, text_y, chars);
      }
    }

    y += line_height_;
  }
}

void TextWindow::DrawTextLine(Renderer& renderer, const TextLine* line, int x, int y) {
  const std::wstring& line_data = line->data();
  if (line_data.empty()) {
    return;
  }

  // For calculating spaces occupied by a tab.
  Coord chars = 0;

#if 0
  // Disabled on 20150908
  if (!ft_plugin()->IsLexAvailable()) {
    Coord j = line->Length();
    DrawTextLinePiece(renderer, line_data, 0, j, Lex(), x, y, chars);
    return;
  }
#endif  // 0

  int _x = x;
  int _y = y;

  const std::list<LexElem*>& lex_elems = line->lex_elems();

  Coord i = 0;
  Coord j = 0;

  std::list<LexElem*>::const_iterator le_it = lex_elems.begin();
  for (; le_it != lex_elems.end(); ++le_it) {
    const LexElem* le = *le_it;

    if (i < le->off) {
      // Line piece (spaces, operators, plain-text, etc.) with no lex.
      DrawTextLinePiece(renderer, line_data, i, le->off, Lex(), _x, _y, chars);
    }

    i = le->off;
    j = i + le->len;
    DrawTextLinePiece(renderer, line_data, i, j, le->lex, _x, _y, chars);

    i = j;
  }

  // Draw line piece after the last lex element.
  j = line->Length();
  if (i < j) {
    // Line piece (spaces, operators, plain-text, etc.) with no lex element.
    DrawTextLinePiece(renderer, line_data, i, j, Lex(), _x, _y, chars);
  }
}

void TextWindow::DrawTextLinePart(Renderer& renderer,
                                  const TextLine* line,
                                  const CharRange& part,
                                  int& x,
                                  int y,
                                  Coord& chars) {
  if (part.IsEmpty()) {
    return;
  }

  const std::wstring& line_data = line->data();

  Coord i = part.begin();
  Coord j = i;

  std::list<const LexElem*> lex_elems = line->lex_elems(part);

  for (const LexElem* le : lex_elems) {
    if (i < le->off) {
      // Line piece (spaces, operators, plain-text, etc.) with no lex.
      DrawTextLinePiece(renderer, line_data, i, le->off, Lex(), x, y, chars);
    }

    i = le->off;
    j = i + le->len;

    if (i < part.begin()) {
      i = part.begin();
    }
    if (j > part.end()) {
      j = part.end();
    }

    DrawTextLinePiece(renderer, line_data, i, j, le->lex, x, y, chars);
    i = j;
  }

  // Draw line piece after the last lex element.
  j = part.end();
  if (i < j) {
    DrawTextLinePiece(renderer, line_data, i, j, Lex(), x, y, chars);
  }
}

void TextWindow::DrawTextLineMatchingPart(Renderer& renderer,
                                          const TextLine* line,
                                          const CharRange& part,
                                          int& x,
                                          int y,
                                          Coord& chars) {
  if (part.IsEmpty()) {
    return;
  }

  const std::wstring& line_data = line->data();

  Coord i = part.begin();
  Coord j = part.end();

  const StyleValue* style_value = style_->Get(Style::kMatching);

  if (style_value->bg().IsOk()) {
    int x1 = x;
    int w = GetLineWidth(line, i, j);
    renderer.SetStyle(style_value->bg(), style_value->bg(), true);
    renderer.DrawRectangle(x1, y - line_padding_, w, line_height_);
    renderer.RestoreStyle();
  }

  DrawTextLinePiece(renderer, line_data, i, j, style_value, false, x, y, chars);
}

void TextWindow::DrawTextLinePiece(Renderer& renderer,
                                   const std::wstring& line_data,
                                   Coord i,
                                   Coord j,
                                   const StyleValue* style_value,
                                   bool has_lex,
                                   int& x,
                                   int y,
                                   Coord& chars) {
  assert(style_value != NULL);
  SetRendererStyle(renderer, style_value);

  wxPen space_pen(style_->Get(Style::kSpace)->fg());

  Coord p = i;

  for (; i < j; ++i) {
    if (line_data[i] != kSpaceChar &&
        line_data[i] != kTabChar &&
        !ft_plugin()->IsDelimiter(line_data[i])) {
      ++chars;
      continue;
    }

    // Render the line piece before this space, tab or operator.
    if (i > p) {
      DrawTextWord(renderer, line_data, p, i - p, x, y, chars);
    }

    // Render the space(s) or tab(s).
    if (line_data[i] == kSpaceChar) {
      Coord spaces = CountCharAfter(line_data, i, kSpaceChar) + 1;
      if (view_options_.show_space) {
        renderer.SetPen(space_pen, false);
        renderer.DrawWhiteSpaces(x, y, spaces);
      }
      x += char_size_.x * spaces;
      chars += spaces;
      i += spaces - 1;
      p = i + 1;
    } else if (line_data[i] == kTabChar) {
      // The tab might occupy "< tab_stop" spaces.
      int tab_spaces = tab_stop_ - (chars % tab_stop_);
      chars += tab_spaces;

      int tab_w = char_size_.x * tab_spaces;
      if (view_options_.show_space) {
        renderer.SetPen(space_pen, false);
        renderer.DrawTab(x, y, tab_w, char_size_.y);
      }
      x += tab_w;

      p = i + 1;
    } else {  // Delimiter
      // Count delimiters.
      Coord delimiters = 1;
      for (Coord k = i + 1;
           k < j && ft_plugin()->IsDelimiter(line_data[k]);
           ++k) {
        ++delimiters;
      }

      if (!has_lex) {
        SetRendererStyle(renderer, style_->Get(Style::kDelimiter));
      } else {
        SetRendererStyle(renderer, style_value);
      }

      int piece_w = 0;
      renderer.DrawText(line_data, i, delimiters, x, y, &piece_w);
      x += piece_w;

      if (!has_lex) {
        // Restore style.
        SetRendererStyle(renderer, style_value);
      }

      chars += delimiters;
      i += delimiters - 1;
      p = i + 1;
    }
  }

  // Render the last line piece if any.
  if (p < j) {
    DrawTextWord(renderer, line_data, p, j - p, x, y, chars);
  }
}

void TextWindow::DrawTextLinePiece(Renderer& renderer,
                                   const std::wstring& line_data,
                                   Coord i,
                                   Coord j,
                                   Lex lex,
                                   int& x,
                                   int y,
                                   Coord& chars) {
  const StyleValue* style_value = style_->Get(lex);
  bool has_lex = !lex.IsEmpty();
  DrawTextLinePiece(renderer, line_data, i, j, style_value, has_lex, x, y, chars);
}

void TextWindow::DrawTextWord(Renderer& renderer,
                              const std::wstring& line_data,
                              Coord off,
                              Coord len,
                              int& x,
                              int y,
                              Coord& chars) {
  int piece_w = 0;
  renderer.DrawText(line_data, off, len, x, y, &piece_w);
  x += piece_w;
}

void TextWindow::SetRendererStyle(Renderer& renderer, const StyleValue* style_value) {
  if (style_value != NULL) {
    // Font
    wxFont font = text_area_->GetFont();
    if ((style_value->font() & Style::kBold) != 0) {
      font.MakeBold();
    }
    if ((style_value->font() & Style::kItalic) != 0) {
      font.MakeItalic();
    }
    renderer.SetFont(font);

    // FG
    if (style_value->fg().IsOk()) {
      renderer.SetTextForeground(style_value->fg());
    } else {
      renderer.SetTextForeground(style_->Get(Style::kNormal)->fg());
    }

    // BG (TODO)
    //renderer.SetTextBackground(style_value->bg());
  }
}

void TextWindow::HandleTextLeftDown(wxMouseEvent& evt) {
  text_area_->SetFocus();
  if (!text_area_->HasCapture()) {
    text_area_->CaptureMouse();
  }

  bool vspace = evt.AltDown();
  down_point_ = CalcCaretPoint(evt.GetPosition(), vspace);

  if (evt.CmdDown()) {
    if (evt.AltDown() && evt.ShiftDown()) {
      HandleTextLeftDown_CtrlAltShift();
    } else if (evt.AltDown()) {
      HandleTextLeftDown_CtrlAlt();
    } else if (evt.ShiftDown()) {
      HandleTextLeftDown_CtrlShift();
    } else {
      HandleTextLeftDown_Ctrl();
    }
  } else {
    if (evt.AltDown() && evt.ShiftDown()) {
      HandleTextLeftDown_AltShift();
    } else if (evt.AltDown()) {
      HandleTextLeftDown_Alt();
    } else if (evt.ShiftDown()) {
      HandleTextLeftDown_Shift();
    } else {
      HandleTextLeftDown_NoAccel();
    }
  }
}

void TextWindow::HandleTextLeftDown_NoAccel() {
  ClearSelection();
  UpdateCaretPoint(down_point_, false, false, false);
}

void TextWindow::HandleTextLeftDown_Ctrl() {
  if (selection_.range.Contain(down_point_)) {
    // Click on selected text, select the bracket pair range.
    TextRange range;

    if (buffer_->IsBracketPairInnerRange(selection_.range)) {
      // If it's already inner range, increase to outer range.
      range = buffer_->IncreaseRange(selection_.range);
    } else {
      // Get inner range.
      range = buffer_->BracketPairInnerRange(down_point_);
    }

    if (!range.IsEmpty()) {
      SetSelection(range.point_begin(), range.point_end(), false);
    }
  } else {  // Click on unselected text.
    // Select the word under the down point.
    SelectByWord(down_point_, down_point_);
  }

  down_modifiers_ = wxMOD_CONTROL;
}

void TextWindow::HandleTextLeftDown_CtrlAlt() {
  ClearSelection();

  down_modifiers_ = wxMOD_CONTROL | wxMOD_ALT;
}

void TextWindow::HandleTextLeftDown_CtrlShift() {
  ExtendSelectionByWord(down_point_);

  down_modifiers_ = wxMOD_CONTROL | wxMOD_SHIFT;
}

void TextWindow::HandleTextLeftDown_CtrlAltShift() {
  down_modifiers_ = wxMOD_CONTROL | wxMOD_ALT | wxMOD_SHIFT;
}

void TextWindow::HandleTextLeftDown_Alt() {
  ClearSelection();

#if JIL_ENABLE_RECT_SELECT
  // Allow virtual spaces when ALT is pressed.
  UpdateCaretPoint(down_point_, false, false, true);
#endif  // JIL_ENABLE_RECT_SELECT

  down_modifiers_ = wxMOD_ALT;
}

void TextWindow::HandleTextLeftDown_Shift() {
  // Extend the selection to the down point by word.
  ExtendSelection(down_point_, false);

  down_modifiers_ = wxMOD_SHIFT;
}

void TextWindow::HandleTextLeftDown_AltShift() {
#if JIL_ENABLE_RECT_SELECT
  // Extend the selection to the down point by rect.
  ExtendSelection(down_point_, true);
#endif  // JIL_ENABLE_RECT_SELECT

  down_modifiers_ = wxMOD_ALT | wxMOD_SHIFT;
}

void TextWindow::HandleTextMotion(wxMouseEvent& evt) {
  if (!text_area_->HasCapture()) {
    return;
  }
  if (!evt.LeftIsDown()) {
    return;
  }

  dragged_ = true;

  move_position_ = evt.GetPosition();
  wxRect client_rect = text_area_->GetClientRect();

  if (move_position_.y > client_rect.GetBottom()) {
    scroll_dir_ = kScrollDown;
    StartScrollTimer();
  } else if (move_position_.y < client_rect.GetTop()) {
    scroll_dir_ = kScrollUp;
    StartScrollTimer();
  } else {
    StopScrollTimer();
  }

  SelectByDragging();
}

void TextWindow::SelectByDragging() {
  if ((down_modifiers_ & wxMOD_ALT) != 0) {
#if JIL_ENABLE_RECT_SELECT
    // Select by rectangle. (TODO)
    TextPoint move_point = CalcCaretPoint(move_position_, true);
    if (move_point != caret_point_) {
      ExtendSelection(move_point, true);
    }
#endif  // JIL_ENABLE_RECT_SELECT
  } else {
    TextPoint move_point = CalcCaretPoint(move_position_, false);

    if ((down_modifiers_ & wxMOD_CONTROL) != 0) {
      // Select text by word.
      bool fw = move_point >= down_point_;
      TextPoint from = buffer_->Seek(down_point_, kWord, fw ? kBegin : kEnd);
      TextPoint to = buffer_->Seek(move_point, kWord, fw ? kEnd : kBegin);
      SetSelection(from, to, false);
    } else {
      // Select text by char.
      if (move_point != caret_point_) {
        ExtendSelection(move_point, false);
      }
    }
  }
}

void TextWindow::StartScrollTimer() {
  // Calculate the speed rate according to distance between
  wxRect client_rect = GetTargetWindow()->GetClientRect();
  int speed_rate = 0;
  if (scroll_dir_ == kScrollDown) {
    speed_rate = (move_position_.y - client_rect.GetBottom()) / 3;
  } else {
    speed_rate = (client_rect.GetTop() - move_position_.y) / 3;
  }
  if (speed_rate < 1) {
    speed_rate = 1;
  }

  int interval = 200 / speed_rate;
  if (interval < 10) {
    interval = 10;
  }

  if (scroll_timer_ == NULL) {
    scroll_timer_ = new wxTimer(this, ID_SCROLL_TIMER);
  }

  if (scroll_timer_->IsRunning()) {
    if (interval != scroll_timer_->GetInterval()) {
      // Stop to change the interval.
      scroll_timer_->Stop();
    } else {
      return;
    }
  }

  scroll_timer_->Start(interval, wxTIMER_CONTINUOUS);

  if (scroll_dir_ == kScrollDown) {
    ScrollLines(1);
  } else if (scroll_dir_ == kScrollUp) {
    ScrollLines(-1);
  }
}

void TextWindow::StopScrollTimer() {
  if (scroll_timer_ != NULL) {
    if (scroll_timer_->IsRunning()) {
      scroll_timer_->Stop();
    }
    wxDELETE(scroll_timer_);
  }
}

void TextWindow::OnScrollTimer(wxTimerEvent& evt) {
  if (scroll_dir_ == kScrollDown) {
    ScrollLines(1);
  } else if (scroll_dir_ == kScrollUp) {
    ScrollLines(-1);
  }
  SelectByDragging();
}

#if !JIL_USE_WX_CARET

void TextWindow::OnCaretTimer(wxTimerEvent& evt) {
  RefreshTextByLine(caret_point_.y);
  caret_.show = !caret_.show;
}

void TextWindow::StartCaretTimer() {
  if (!caret_.timer->IsRunning()) {
    caret_.timer->Start(caret_.interval_ms, false);
  }
}

void TextWindow::StopCaretTimer() {
  if (caret_.timer->IsRunning()) {
    caret_.timer->Stop();
  }
}

void TextWindow::RestartCaretTimer() {
  if (caret_.timer->IsRunning()) {
    caret_.timer->Stop();
  }

  caret_.show = true;
  caret_.timer->Start(caret_.interval_ms, false);
}

#endif  // !JIL_USE_WX_CARET

void TextWindow::HandleTextLeftUp(wxMouseEvent& evt) {
  if (text_area_->HasCapture()) {
    text_area_->ReleaseMouse();
  }

  StopScrollTimer();

  // Clear flags.
  down_modifiers_ = 0;
  dragged_ = false;
}

void TextWindow::HandleTextRightDown(wxMouseEvent& evt) {
  text_area_->SetFocus();

  down_point_ = CalcCaretPoint(evt.GetPosition(), false);

  if (!selection_.IsEmpty() && !selection_.Contain(down_point_)) {
    // Clear selection if click outside of it.
    ClearSelection();
  }

  UpdateCaretPoint(down_point_, false, false, false);
}

void TextWindow::HandleTextRightUp(wxMouseEvent& evt) {
}

void TextWindow::HandleTextLeftDClick(wxMouseEvent& evt) {
  // Double-click selects the clicked word.
  TextPoint point = CalcCaretPoint(evt.GetPosition(), false);
  SelectByWord(point, point);
}

bool TextWindow::HandleTextMouseWheel(wxMouseEvent& evt) {
  if (evt.ControlDown() && !font_range_.IsEmpty()) {
    wxFont font = text_area_->GetFont();
    int size = font.GetPointSize();

    if (evt.GetWheelRotation() > 0) {
      ++size;
    } else if (evt.GetWheelRotation() < 0) {
      --size;
    }

    if (size >= font_range_.min && size <= font_range_.max) {
      font.SetPointSize(size);
      SetTextFont(font);
    }

    return true;
  }

  if (evt.ShiftDown()) {
    ScrollHorizontally(evt);
    return true;
  }

  return false;  // For default handling.
}

void TextWindow::ScrollHorizontally(wxMouseEvent& evt) {
  int view_start_x = 0;
  GetViewStart(&view_start_x, NULL);

  int x = wxDefaultCoord;

  if (evt.GetWheelRotation() > 0) {
    if (view_start_x > 0) {
      x = view_start_x - evt.GetLinesPerAction();
      if (x < 0) {
        x = 0;
      }
    }
  } else if (evt.GetWheelRotation() < 0) {
    x = view_start_x + evt.GetLinesPerAction();
  }

  if (x != wxDefaultCoord) {
    Scroll(x, wxDefaultCoord);
  }
}

void TextWindow::OnTextMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  // Stop select-by-dragging.
  StopScrollTimer();
}

// About the return value:
// - true: the event won't be skipped
// - false: the event will be skipped
// If the key down event is not skipped, no char event.
bool TextWindow::OnTextKeyDown(wxKeyEvent& evt) {
  int code = evt.GetKeyCode();
  int modifiers = evt.GetModifiers();

  if (code == WXK_NONE) {
    return false;
  }

  if (code == WXK_CONTROL || code == WXK_SHIFT || code == WXK_ALT) {
    return false;
  }

  if (code == WXK_ESCAPE) {
    // Clear find result.
    if (!find_result_.IsEmpty()) {
      ClearFindResult();
      return true;
    }

    // Or clear current selection.
    if (!selection_.IsEmpty()) {
      ClearSelection();
      return true;
    }

    // NOTE: ESCAPE is supported to be used in the key binding.
    return false;
  }

#if JIL_ENABLE_LEADER_KEY
  if (code == WXK_TAB && modifiers == 0 && leader_key_->IsEmpty()) {
#else
  if (code == WXK_TAB && modifiers == 0) {
#endif  // JIL_ENABLE_LEADER_KEY
    if (allow_text_change_) {
      // Input tab (expand or not).
      if (expand_tab_) {
        int spaces = tab_stop_ - (caret_point_.x % tab_stop_);
        // ISSUE: Continuous tabs cannot be undone together.
        InsertString(std::wstring(spaces, kSpaceChar));
      } else {
        InsertChar(kTabChar);
      }
    }
    return true;
  }

  // Avoid matching for single character key.
#if JIL_ENABLE_LEADER_KEY
  if (leader_key_->IsEmpty()) {
#endif  // JIL_ENABLE_LEADER_KEY
    if (modifiers == 0) {
      // Standard ASCII characters || ASCII extended characters.
      if ((code >= 33 && code <= 126) || (code >= 128 && code <= 255)) {
        return false;
      }
    } else if (modifiers == wxMOD_SHIFT && code < 127) {
      if (editor::kNonShiftChars.find(static_cast<char>(code)) != std::string::npos) {
        // ~!@#$%^&*()_+<>?:"{}|
        return false;
      }
    }
#if JIL_ENABLE_LEADER_KEY
  }
#endif  // JIL_ENABLE_LEADER_KEY

  editor::Key key(code, modifiers);

#if JIL_ENABLE_LEADER_KEY
  if (!leader_key_->IsEmpty()) {
    key.set_leader(*leader_key_);
  }
#endif  // JIL_ENABLE_LEADER_KEY

  // Get text function by key.
  TextFunc* text_func = binding_->GetTextFuncByKey(key, mode());

#if JIL_ENABLE_LEADER_KEY
  // Reset leader key no matter if the text function is found or not.
  if (!leader_key_->IsEmpty()) {
    leader_key_->Reset();
    PostEvent(kLeaderKeyEvent);
  }
#endif  // JIL_ENABLE_LEADER_KEY

  if (text_func != NULL) {
    if (allow_text_change_ || !text_func->change_text()) {
      text_func->Exec(this);
    }
    return true;
  }

  return false;
}

void TextWindow::OnTextChar(wxKeyEvent& evt) {
  if (!allow_text_change_) {
    return;
  }

  if (evt.AltDown() || evt.CmdDown()) {
    evt.Skip();
    return;
  }

  wchar_t c = evt.GetUnicodeKey();
  if (c >= 0x20) {  // "< 0x20" is CONTROL
    InsertChar(c);
  }
}

void TextWindow::OnTextSetFocus(wxFocusEvent& evt) {
  PostEvent(kGetFocusEvent);

  // Restore the caret blinking timer.
  StartCaretTimer();
}

void TextWindow::OnTextKillFocus(wxFocusEvent& evt) {
  // Don't show caret when focus is lost.
  StopCaretTimer();
}

//------------------------------------------------------------------------------
// Delegated event handlers from LineNrArea.

// Paint the line number area.
void TextWindow::OnLineNrPaint(wxDC& dc) {
  if (!view_options_.wrap) {
    HandleLineNrPaint(dc);
  } else {
    HandleWrappedLineNrPaint(dc);
  }
}

void TextWindow::HandleLineNrPaint(wxDC& dc) {
  assert(!view_options_.wrap);

  // Get the lines to paint.
  wxRect update_rect = line_nr_area_->GetUpdateClientRect();
  LineRange update_line_range = LineRangeFromClientRect(update_rect);
  if (update_line_range.IsEmpty()) {
    return;  // No lines to paint.
  }

  const Coord line_count = buffer_->LineCount();

  LineRange blank_line_range;

  if (update_line_range.first() > line_count) {
    blank_line_range = update_line_range;
    update_line_range.Set(0);  // No text to draw.
  } else {
    if (update_line_range.last() > line_count) {
      blank_line_range.Set(line_count + 1, update_line_range.last());
      update_line_range.set_last(line_count);  // Remove the blank lines.
    }
  }

  const wxRect client_rect = line_nr_area_->GetClientRect();
  int x = client_rect.GetLeft() + GetUnscrolledX(0);

  if (!update_line_range.IsEmpty()) {
    // Caret line highlight.
    if (update_line_range.Has(caret_point_.y)) {
      const wxColour& bg = style_->Get(Style::kCaretLine)->bg();

      dc.SetPen(wxPen(bg));
      dc.SetBrush(wxBrush(bg));

      int y = client_rect.y + line_height_ * (caret_point_.y - 1);
      int w = client_rect.width - kLineNrHlPaddingRight;

      dc.DrawRectangle(x, y, w, line_height_);
    }

    // Draw right aligned line numbers.
    if (view_options_.show_number) {
      dc.SetFont(line_nr_area_->GetFont());
      dc.SetTextForeground(style_->Get(Style::kNumber)->fg());

      int y = client_rect.y + line_height_ * (update_line_range.first() - 1);
      int w = client_rect.width - kLineNrPaddingRight;
      wxRect rect(x, y, w, line_height_);

      wxString label;
      for (Coord ln = update_line_range.first(); ln <= update_line_range.last(); ++ln) {
        label.Printf(_T("%d"), ln);
        dc.DrawLabel(label, rect, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        rect.y += line_height_;
      }
    }
  }

  // Blank lines.
  if (!blank_line_range.IsEmpty()) {
    const wxColour& blank_bg = style_->Get(Style::kBlank)->bg();

    int y = client_rect.y + line_height_ * (blank_line_range.first() - 1);

    // Draw a different background for blank lines.
    if (blank_bg != style_->Get(Style::kNumber)->bg()) {
      dc.SetPen(wxPen(blank_bg));
      dc.SetBrush(wxBrush(blank_bg));

      int w = client_rect.width;
      int h = line_height_ * blank_line_range.LineCount();

      dc.DrawRectangle(x, y, w, h);
    }

    // Draw a tilde for each blank line.
    if (!view_options_.show_number || update_line_range.IsEmpty()) {
      // Font is not set yet.
      dc.SetFont(line_nr_area_->GetFont());
    }

    dc.SetTextForeground(style_->Get(Style::kBlank)->fg());

    int w = client_rect.width - kLineNrPaddingRight;
    wxRect rect(x, y, w, line_height_);

    Coord ln = blank_line_range.first();
    for (; ln <= blank_line_range.last(); ++ln) {
      dc.DrawLabel(kTilde, rect, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
      rect.y += line_height_;
    }
  }
}

void TextWindow::HandleWrappedLineNrPaint(wxDC& dc) {
  assert(view_options_.wrap);

  // Get the lines to paint.
  wxRect rect = line_nr_area_->GetUpdateClientRect();
  LineRange wrapped_line_range(LineFromScrolledY(rect.GetTop()),
                               LineFromScrolledY(rect.GetBottom()));
  if (wrapped_line_range.IsEmpty()) {
    return;  // No lines to paint.
  }

  const Coord wrapped_line_count = wrap_helper()->WrappedLineCount();

  LineRange blank_line_range;

  if (wrapped_line_range.first() > wrapped_line_count) {
    blank_line_range = wrapped_line_range;
    wrapped_line_range.Set(0);  // No text to draw.
  } else {
    if (wrapped_line_range.last() > wrapped_line_count) {
      blank_line_range.Set(wrapped_line_count + 1, wrapped_line_range.last());
      // Remove the blank lines.
      wrapped_line_range.set_last(wrapped_line_count);
    }
  }

  const wxRect client_rect = line_nr_area_->GetClientRect();
  int x = client_rect.GetLeft() + GetUnscrolledX(0);

  if (!wrapped_line_range.IsEmpty()) {
    // Unwrap the line range.
    LineRange line_range = wrap_helper()->UnwrapLineRange(wrapped_line_range);

    // Caret line highlight.
    if (line_range.Has(caret_point_.y)) {
      const wxColour& bg = style_->Get(Style::kCaretLine)->bg();
      dc.SetPen(wxPen(bg));
      dc.SetBrush(wxBrush(bg));

      Coord first_ln = wrap_helper()->WrapLineNr(caret_point_.y);
      int y = client_rect.GetTop() + line_height_ * (first_ln - 1);
      int w = client_rect.GetWidth() - kLineNrHlPaddingRight;
      int h = line_height_ * wrap_helper()->WrappedLineCount(caret_point_.y);

      dc.DrawRectangle(x, y, w, h);
    }

    // Draw right aligned line numbers.
    if (view_options_.show_number) {
      dc.SetFont(line_nr_area_->GetFont());
      dc.SetTextForeground(style_->Get(Style::kNumber)->fg());

      Coord first_ln = wrap_helper()->WrapLineNr(line_range.first());
      int y = client_rect.GetTop() + line_height_ * (first_ln - 1);
      int w = client_rect.GetWidth() - kLineNrPaddingRight;
      wxRect rect(x, y, w, line_height_);

      wxString label;
      for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
        label.Printf(_T("%d"), ln);
        dc.DrawLabel(label, rect, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        rect.y += line_height_ * wrap_helper()->WrappedLineCount(ln);
      }
    }
  }

  if (!blank_line_range.IsEmpty()) {
    const wxColour& blank_bg = style_->Get(Style::kBlank)->bg();

    int y = client_rect.y + line_height_ * (blank_line_range.first() - 1);

    // Draw a different background for blank lines.
    if (blank_bg != style_->Get(Style::kNumber)->bg()) {
      dc.SetPen(wxPen(blank_bg));
      dc.SetBrush(wxBrush(blank_bg));

      int h = line_height_ * blank_line_range.LineCount();
      int w = client_rect.width;

      dc.DrawRectangle(x, y, w, h);
    }

    // Draw a tilde for each blank line.
    if (!view_options_.show_number || wrapped_line_range.IsEmpty()) {
      // Font is not set yet.
      dc.SetFont(line_nr_area_->GetFont());
    }

    dc.SetTextForeground(style_->Get(Style::kBlank)->fg());

    int w = client_rect.width - kLineNrPaddingRight;
    wxRect rect(x, y, w, line_height_);

    Coord ln = blank_line_range.first();
    for (; ln <= blank_line_range.last(); ++ln) {
      dc.DrawLabel(kTilde, rect, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
      rect.y += line_height_;
    }
  }
}

void TextWindow::OnLineNrSize(wxSizeEvent& evt) {
  line_nr_area_->Refresh();
}

bool TextWindow::OnLineNrMouse(wxMouseEvent& evt) {
  bool handled = true;

  wxEventType evt_type = evt.GetEventType();

  if (evt_type == wxEVT_LEFT_DOWN) {
    HandleLineNrLeftDown(evt);
  } else if (evt_type == wxEVT_MOTION) {
    HandleLineNrMotion(evt);
  } else if (evt_type == wxEVT_LEFT_UP) {
    HandleLineNrLeftUp(evt);
  } else {
    handled = false;
  }

  return handled;
}

void TextWindow::HandleLineNrLeftDown(wxMouseEvent& evt) {
  if (!line_nr_area_->HasCapture()) {
    line_nr_area_->CaptureMouse();
  }

  down_point_ = CalcCaretPoint(evt.GetPosition(), false);

  if (!evt.ShiftDown()) {
    SelectByLine(down_point_.y, down_point_.y);
  } else {
    ExtendSelectionByLine(down_point_.y);
  }
}

void TextWindow::HandleLineNrMotion(wxMouseEvent& evt) {
  if (!line_nr_area_->HasCapture()) {
    return;
  }
  if (!evt.LeftIsDown()) {
    return;
  }

  TextPoint move_point = CalcCaretPoint(evt.GetPosition(), false);
  SelectByLine(down_point_.y, move_point.y);
}

void TextWindow::HandleLineNrLeftUp(wxMouseEvent& evt) {
  if (line_nr_area_->HasCapture()) {
    line_nr_area_->ReleaseMouse();
  }
}

//------------------------------------------------------------------------------

Coord TextWindow::LineFromScrolledY(int scrolled_y) const {
  return (GetUnscrolledY(scrolled_y) + line_height_) / line_height_;
}

TextPoint TextWindow::CalcCaretPoint(const wxPoint& pos, bool vspace) {
  // Adjust the position if it's out of the text client area. This happens
  // when select text by dragging mouse.
  const wxRect client_rect = text_area_->GetClientRect();
  wxPoint adjusted_pos = pos;
  if (adjusted_pos.y > client_rect.GetBottom()) {
    adjusted_pos.y = client_rect.GetBottom();
  } else if (adjusted_pos.y < client_rect.GetTop()) {
    adjusted_pos.y = client_rect.GetTop();
  }

  // Calculate unscrolled position.
  wxPoint unscrolled_pos = GetUnscrolledPos(adjusted_pos);

  // Calculate caret point from unscrolled position.
  TextPoint caret_point;
  caret_point.y = unscrolled_pos.y / line_height_;
  if (unscrolled_pos.y % line_height_ > 0) {
    ++caret_point.y;
  }
  if (caret_point.y < 1) {
    caret_point.y = 1;
  }

  if (!view_options_.wrap) {
    if (caret_point.y > buffer_->LineCount()) {
      caret_point.y = buffer_->LineCount();
    }
    caret_point.x = GetCharIndex(caret_point.y, unscrolled_pos.x, vspace);
  } else {
    Coord wrapped_ln = caret_point.y;
    Coord wrapped_line_count = wrap_helper()->WrappedLineCount();
    if (wrapped_ln > wrapped_line_count) {
      wrapped_ln = wrapped_line_count;
    }

    Coord sub_ln = 0;
    Coord ln = wrap_helper()->UnwrapLineNr(wrapped_ln, &sub_ln);

    caret_point.y = ln;
    caret_point.x = GetWrappedCharIndex(caret_point.y, sub_ln, unscrolled_pos.x, vspace);
  }

  return caret_point;
}

wxRect TextWindow::ClientRectFromLineRange(wxWindow* area, const LineRange& line_range) const {
  wxRect client_rect = area->GetClientRect();

  Coord line_first = 1;
  Coord line_count = 1;

  if (view_options_.wrap) {
    LineRange line_range_copy = wrap_helper()->WrapLineRange(line_range);
    line_first = line_range_copy.first();
    line_count = line_range_copy.LineCount();
  } else {
    line_first = line_range.first();
    line_count = line_range.LineCount();
  }

  int scrolled_y = client_rect.y + GetScrolledY(line_height_ * (line_first - 1));

  return wxRect(client_rect.x,
                scrolled_y,
                client_rect.width,
                line_height_ * line_count);
}

LineRange TextWindow::LineRangeFromClientRect(const wxRect& client_rect) const {
  return LineRange(LineFromScrolledY(client_rect.GetTop()),
                   LineFromScrolledY(client_rect.GetBottom()));
}

wxRect TextWindow::ClientRectAfterLine(wxWindow* area, Coord ln, bool included) const {
  Coord v_ln = ln;
  if (view_options_.wrap) {
    v_ln = wrap_helper()->WrapLineNr(ln);
  }

  if (!included) {
    if (!view_options_.wrap) {
      --v_ln;
    } else {
      v_ln -= wrap_helper()->WrappedLineCount(ln);
    }
  }

  int y = GetScrolledY(line_height_ * (v_ln - 1));

  const wxRect rect = area->GetClientRect();

  if (y <= rect.GetTop()) {
    return rect;
  } else if (y > rect.GetBottom()) {
    return wxRect();  // Empty
  } else {
    return wxRect(rect.GetLeft(), y, rect.GetWidth(), rect.GetHeight() - y);
  }
}

//------------------------------------------------------------------------------
// Refresh

void TextWindow::RefreshTextAfterLine(Coord ln, bool included, bool update) {
  wxRect rect = ClientRectAfterLine(text_area_, ln, included);
  if (!rect.IsEmpty()) {
    text_area_->RefreshRect(rect);
    if (update) {
      text_area_->Update();
    }
  }
}

void TextWindow::RefreshLineNrAfterLine(Coord ln, bool included, bool update) {
  wxRect rect = ClientRectAfterLine(line_nr_area_, ln, included);
  if (!rect.IsEmpty()) {
    line_nr_area_->RefreshRect(rect);
    if (update) {
      line_nr_area_->Update();
    }
  }
}

void TextWindow::RefreshTextByLine(Coord ln, bool update) {
  if (ln <= buffer_->LineCount()) {
    RefreshTextByLineRange(LineRange(ln), update);
  }
}

void TextWindow::RefreshTextByLineRange(const LineRange& line_range, bool update) {
  wxRect refresh_rect = ClientRectFromLineRange(text_area_, line_range);
  text_area_->RefreshRect(refresh_rect);
  if (update) {
    text_area_->Update();
  }
}

void TextWindow::RefreshLineNrByLine(Coord ln, bool update) {
  if (ln <= buffer_->LineCount()) {
    RefreshLineNrByLineRange(LineRange(ln), update);
  }
}

void TextWindow::RefreshLineNrByLineRange(const LineRange& line_range, bool update) {
  wxRect rect = ClientRectFromLineRange(line_nr_area_, line_range);
  line_nr_area_->RefreshRect(rect);
  if (update) {
    line_nr_area_->Update();
  }
}

//------------------------------------------------------------------------------

bool TextWindow::HandleTextChange() {
  UpdateTextSize();

  // Update line number area width since the number of lines might change.
  int old_line_nr_width = line_nr_width_;
  UpdateLineNrWidth();

  bool resized = false;

  // If the line number width changes, layout the areas.
  if (old_line_nr_width != line_nr_width_) {
    LayoutAreas();
    resized = true;
  }

  // If resized (LayoutAreas() is called), there will be size event triggered,
  // and in the size event handler, UpdateVirtualSize() will be called.
  // See OnTextSize().
  if (!resized) {
    UpdateVirtualSize();
  }

  return resized;
}

void TextWindow::UpdateLineHeight() {
  line_height_ = line_padding_ + char_size_.y + line_padding_;
}

void TextWindow::HandleLineHeightChange() {
  SetScrollbars(char_size_.x, line_height_, 1, 1);

  UpdateTextSize();
  UpdateVirtualSize();

#if JIL_USE_WX_CARET
  // Update caret size and position.
  int caret_height = line_height_;

#if defined (__WXOSX__)
  --caret_height;
#endif

  text_area_->GetCaret()->SetSize(kCaretWidth, caret_height);

#endif  // JIL_USE_WX_CARET

  UpdateCaretPosition();
}

void TextWindow::UpdateTextSize() {
  if (!view_options_.wrap) {
    text_size_.x = char_size_.x * buffer_->GetMaxLineLength();
    text_size_.y = line_height_ * buffer_->LineCount();
  } else {
    text_size_.x = -1;
    text_size_.y = line_height_ * wrap_helper()->WrappedLineCount();
  }
}

void TextWindow::UpdateLineNrWidth() {
  if (view_options_.show_number) {
    wxString line_nr_str = wxString::Format(L"%d", buffer_->LineCount());
    line_nr_area_->GetTextExtent(line_nr_str, &line_nr_width_, NULL);
    line_nr_width_ += kLineNrPadding;
  } else {
    int tilde_width = 0;
    line_nr_area_->GetTextExtent(kTilde, &tilde_width, NULL);
    line_nr_width_ = tilde_width + kLineNrPadding;
  }
}

void TextWindow::UpdateVirtualSize() {
  Coord line_count = 0;
  if (!view_options_.wrap) {
    line_count = buffer_->LineCount();
  } else {
    line_count = wrap_helper()->WrappedLineCount();
  }

  // -1 to keep the last line visible when scroll to the end.
  int vh = line_height_ * (line_count + GetPageSize() - 1);

  if (!view_options_.wrap) {
    // - char_size_.x to keep the last char visible when scroll to the end.
    int vw = text_size_.x + GetTextClientWidth() - char_size_.x;
    text_area_->SetVirtualSize(vw, vh);
  } else {
    text_area_->SetVirtualSize(-1, vh);
  }

  line_nr_area_->SetVirtualSize(-1, vh);

  AdjustScrollbars();
}

void TextWindow::LayoutAreas() {
  const wxRect client_rect = GetClientRect();

  int text_area_height = client_rect.height - kMarginTop - kMarginBottom - kTextMarginBottom;

  line_nr_area_->SetSize(0, kMarginTop, line_nr_width_, text_area_height);

  text_area_->SetSize(line_nr_width_ + kTextMarginLeft,
                      kMarginTop,
                      client_rect.width - kTextMarginLeft - line_nr_width_,
                      text_area_height);
}

//------------------------------------------------------------------------------
// Caret

void TextWindow::UpdateCaretPosition() {
  if (!caret_point_.Valid()) {
    return;
  }

  Coord x_off = 0;
  int y = 0;

  if (!view_options_.wrap) {
    y = caret_point_.y;
  } else {
    Coord sub_ln = wrap_helper()->SubLineNr(caret_point_.y, caret_point_.x, &x_off);
    // -1 because sub_ln is 1-based.
    y = wrap_helper()->WrapLineNr(caret_point_.y) + sub_ln - 1;
  }

  int unscrolled_x = GetLineWidth(caret_point_.y, x_off, caret_point_.x);
  int unscrolled_y = (y - 1) * line_height_;

#if defined (__WXOSX__)
  ++unscrolled_y;
#endif

#if JIL_USE_WX_CARET
  wxPoint p;
  CalcScrolledPosition(unscrolled_x, unscrolled_y, &p.x, &p.y);
  text_area_->GetCaret()->Move(p);
#else
  caret_.pos.x = unscrolled_x;
  caret_.pos.y = unscrolled_y;
#endif  // JIL_USE_WX_CARET
}

//------------------------------------------------------------------------------
// Selection

void TextWindow::SetSelection(const TextPoint& point_from, const TextPoint& point_to, bool vspace) {
  if (point_from <= point_to) {
    SetSelection(TextRange(point_from, point_to), kForward, vspace);
  } else {
    SetSelection(TextRange(point_to, point_from), kBackward, vspace);
  }
  UpdateCaretPoint(point_to, false, true, vspace);
}

void TextWindow::ExtendSelection(const TextPoint& point_to, bool vspace) {
  TextPoint point_from = caret_point_;
  if (!selection_.IsEmpty()) {
    point_from = selection_.GetFromPoint();
  }
  SetSelection(point_from, point_to, vspace);
}

void TextWindow::ExtendSelectionByWord(const TextPoint& point_to) {
  if (selection_.IsEmpty()) {
    SelectByWord(caret_point_, point_to);
  } else {
    if (selection_.dir == kForward) {
      SelectByWord(selection_.begin(), point_to);
    } else {
      TextPoint point_from = selection_.end();
      if (point_from.x > 0) {
        --point_from.x;
      } else {
        --point_from.y;
      }
      SelectByWord(point_from, point_to);
    }
  }
}

void TextWindow::ExtendSelectionByLine(Coord ln_to) {
  if (selection_.IsEmpty()) {
    SelectByLine(caret_point_.y, ln_to);
  } else {
    if (selection_.dir == kForward) {
      SelectByLine(selection_.begin().y, ln_to);
    } else {
      Coord ln_from = selection_.end().y;
      if (selection_.end().x == 0) {
        --ln_from;
      }
      SelectByLine(ln_from, ln_to);
    }
  }
}

void TextWindow::SelectByWord(const TextPoint& point_from, const TextPoint& point_to) {
  if (point_from <= point_to) {
    TextPoint word_point_begin = buffer_->Seek(point_from, kWord, kBegin);
    TextPoint word_point_end = buffer_->Seek(point_to, kWord, kEnd);
    SetSelection(word_point_begin, word_point_end, false);
  } else {
    TextPoint word_point_begin = buffer_->Seek(point_to, kWord, kBegin);
    TextPoint word_point_end = buffer_->Seek(point_from, kWord, kEnd);
    SetSelection(word_point_end, word_point_begin, false);
  }
}

void TextWindow::SelectByLine(Coord ln_from, Coord ln_to) {
  TextDir dir = kForward;

  if (ln_to < ln_from) {
    std::swap(ln_to, ln_from);
    dir = kBackward;
  }

  TextPoint point_begin(0, ln_from);

  TextPoint point_end;
  if (ln_to < buffer_->LineCount()) {
    point_end.Set(0, ln_to + 1);
  } else {
    point_end.Set(buffer_->LineLength(ln_to), ln_to);
  }

  if (dir == kForward) {
    SetSelection(point_begin, point_end, false);
  } else {
    SetSelection(point_end, point_begin, false);
  }
}

//------------------------------------------------------------------------------

int TextWindow::GetLineWidth(const TextLine* line, Coord off1, Coord off2) const {
  assert(off1 <= line->Length());

  int line_width = 0;

  if (off2 == kInvCoord) {
    off2 = line->Length();
  } else if (off2 > line->Length()) {
    line_width = char_size_.x * (off2 - line->Length());
    off2 = line->Length();
  }

  Coord count = off2 - off1;
  if (count == 0) {
    return 0;
  }

  line_width += text_extent_->GetLineWidth(tab_stop_, line, off1, count);

  return line_width;
}

int TextWindow::GetLineWidth(Coord ln, Coord off1, Coord off2) const {
  return GetLineWidth(buffer_->Line(ln), off1, off2);
}

Coord TextWindow::GetCharIndex(Coord ln, int client_x, bool vspace) const {
  return text_extent_->IndexChar(tab_stop_, buffer_->Line(ln), 0, client_x, vspace);
}

Coord TextWindow::GetWrappedCharIndex(Coord ln, Coord wrapped_sub_ln, int client_x, bool vspace) const {
  assert(view_options_.wrap);
  assert(wrapped_sub_ln >= 1);

  if (wrapped_sub_ln == 1) {
    return GetCharIndex(ln, client_x, vspace);
  } else {  // >= 2
    const WrapInfo& wrap_info = wrap_helper()->GetWrapInfo(ln);
    Coord off = wrap_info.offsets()[wrapped_sub_ln - 2];
    Coord i = text_extent_->IndexChar(tab_stop_, buffer_->Line(ln), off, client_x, vspace);
    return off + i;
  }
}

int TextWindow::GetUnscrolledX(int scrolled_x) const {
  int unscrolled_x = 0;
  CalcUnscrolledPosition(scrolled_x, 0, &unscrolled_x, NULL);
  return unscrolled_x;
}

int TextWindow::GetUnscrolledY(int scrolled_y) const {
  int unscrolled_y = 0;
  CalcUnscrolledPosition(0, scrolled_y, NULL, &unscrolled_y);
  return unscrolled_y;
}

wxPoint TextWindow::GetUnscrolledPos(const wxPoint& pos) const {
  return CalcUnscrolledPosition(pos);
}

int TextWindow::GetScrolledX(int unscrolled_x) const {
  int scrolled_x = 0;
  CalcScrolledPosition(unscrolled_x, 0, &scrolled_x, NULL);
  return scrolled_x;
}

int TextWindow::GetScrolledY(int unscrolled_y) const {
  int scrolled_y = 0;
  CalcScrolledPosition(0, unscrolled_y, NULL, &scrolled_y);
  return scrolled_y;
}

LineRange TextWindow::GetClientLineRange() const {
  LineRange line_range;
  int unscrolled_y = 0;
  CalcUnscrolledPosition(0, 0, NULL, &unscrolled_y);
  line_range.set_first((unscrolled_y + line_height_) / line_height_);
  line_range.set_last(line_range.first() + GetPageSize() - 1);
  return line_range;
}

//------------------------------------------------------------------------------

void TextWindow::PostEvent(int event_type, void* client_data) {
  wxCommandEvent evt(kTextWindowEvent, GetId());
  evt.SetEventObject(this);
  evt.SetClientData(client_data);
  evt.SetInt(event_type);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

wxMenuItem* TextWindow::AppendMenuItem(wxMenu* menu,
                                       int id,
                                       const wxString& label,
                                       wxItemKind kind) {
  wxString _label = label;

  // Append accelerator.
  editor::Key key = binding_->GetKeyByMenu(id);
  if (!key.IsEmpty()) {
    _label += wxT("\t") + key.ToString();
  }

  wxMenuItem* item = new wxMenuItem(menu, id, _label, wxEmptyString, kind);
  menu->Append(item);

  return item;
}

}  // namespace editor
}  // namespace jil
