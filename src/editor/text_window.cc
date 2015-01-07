#include "editor/text_window.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>
#include "wx/dcbuffer.h"
#include "wx/sizer.h"
#include "wx/caret.h"
#include "wx/timer.h"
#include "wx/log.h"
#include "base/string_util.h"
#include "base/math_util.h"
#include "editor/compile_config.h"
#include "editor/util.h"
#include "editor/style.h"
#include "editor/color.h"
#include "editor/line_nr_area.h"
#include "editor/text_area.h"
#include "editor/text_buffer.h"
#include "editor/wrap.h"
#include "editor/tab.h"
#include "editor/text_extent.h"
#include "editor/renderer.h"
#include "editor/action.h"
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

static const int kMarginTop = 3;
static const int kMarginBottom = 0;
static const int kTextMarginLeft = 0;
static const int kTextMarginBottom = 0;
static const int kLineNrPadding = 15;
static const int kLineNrPaddingRight = 9;
static const int kLineNrHlPaddingRight = 3;
static const int kLinePaddingY = 1;
static const int kCaretWidth = 1;

// Displayed on the blank lines of line number area.
// A respect to Vim.
static const wxString kTilde = wxT("~");

//------------------------------------------------------------------------------

IMPLEMENT_CLASS(TextWindow, wxScrolledWindow)

BEGIN_EVENT_TABLE(TextWindow, wxScrolledWindow)
EVT_SIZE(TextWindow::OnSize)
EVT_SET_FOCUS(TextWindow::OnSetFocus)
END_EVENT_TABLE()

//------------------------------------------------------------------------------

TextWindow::TextWindow(TextBuffer* buffer)
    : buffer_(buffer)
    , options_(buffer->options())
    , style_(NULL)
    , binding_(NULL)
    , line_nr_width_(0)
    , char_width_(0)
    , char_height_(0)
    , line_height_(0)
    , text_width_(0)
    , text_height_(0)
    , caret_point_(0, 1)
    , max_caret_x_(0)
    , wrap_helper_(NULL)
    , down_modifiers_(0)
    , dragged_(false)
    , scroll_timer_(NULL)
    , scroll_dir_(kScrollDown) {
  text_extent_ = new TextExtent;
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

  if (!options_.show_hscrollbar) {
    ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
  }

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
  text_area_->SetBackgroundColour(style_->Get(Style::kNormal)->bg());
  text_area_->SetCursor(wxCURSOR_IBEAM);

  // Set caret for text area.
  wxCaret* caret = new wxCaret(text_area_, kCaretWidth, -1);
  text_area_->SetCaret(caret);
  caret->Show();
  SetTargetWindow(text_area_);

  UpdateLineNrWidth();
  // Update what are determined by text font (char width, line height, etc.).
  HandleTextFontChange();

  // Attach buffer listener when text window is actually created.
  buffer_->AttachListener(this);

  return true;
}

TextWindow::~TextWindow() {
  if (scroll_timer_ != NULL) {
    if (scroll_timer_->IsRunning()) {
      scroll_timer_->Stop();
    }
    delete scroll_timer_;
  }

  buffer_->DetachListener(this);
  delete buffer_;

  if (wrap_helper_ != NULL) {
    delete wrap_helper_;
  }

  delete text_extent_;
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

  HandleTextFontChange();
  text_area_->Refresh();
}

void TextWindow::SetLineNrFont(const wxFont& font) {
  line_nr_area_->SetOwnFont(font);

  HandleLineNrFontChange();
  line_nr_area_->Refresh();
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


void TextWindow::OnBufferLineChange(LineChangeType type,
                                    const LineChangeData& data) {
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

  default:
    break;
  }
}

void TextWindow::OnBufferChange(ChangeType type) {
  switch (type) {
    case kEncodingChange:
      PostEvent(TextWindow::kEncodingEvent);
      break;

    case kFileFormatChange:
      PostEvent(TextWindow::kFileFormatEvent);
      break;

    case kFileNameChange:
      PostEvent(TextWindow::kFileNameEvent);
      break;

    case kModifiedChange:
      PostEvent(TextWindow::kModifiedEvent);
      break;

    default:
      break;
  }
}

void TextWindow::Wrap(bool wrap) {
  if (wrap == options_.wrap) {
    return;
  }
  options_.wrap = wrap;

  int wrap_delta = 0;
  bool wrap_changed = false;

  if (options_.wrap) {
    wrap_changed = wrap_helper()->Wrap(&wrap_delta);
  } else {
    if (wrap_helper_ != NULL) {
      wrap_changed = wrap_helper_->Unwrap(&wrap_delta);
      wxDELETE(wrap_helper_);
    }
  }

  // Virtual height might change due to the wrap change.
  // The wrap-on virtual width is also different from the wrap-off one.
  UpdateVirtualSize();

  if (wrap_changed) {
    text_area_->Refresh();
    if (wrap_delta != 0) {
      line_nr_area_->Refresh();
    }
  }

  // Caret position might change due to the wrap change.
  UpdateCaretPosition();
  ScrollToPoint(caret_point_);
}

void TextWindow::set_leader_key(Key* key) {
  if (leader_key_ != key) {
    leader_key_ = key;
  }
}

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
    SetFileFormatAction* sffa = new SetFileFormatAction(buffer_,
                                                        caret_point_,
                                                        file_format);
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
  if (text_unit == kSelected && seek_type == kWhole) {
    // Clear current selection.
    ClearSelection();
    return;
  }

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
  DeleteRangeAction* dra =
      new DeleteRangeAction(buffer_, range, dir, rect, selected);

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
  if (options_.wrap) {
    caret_y = wrap_helper()->WrapLineNr(caret_y) +
              wrap_helper()->WrappedLineCount(caret_y) -
              1;
  }

  // - 1 because line nr is 0-based.
  if (caret_y - 1 < view_start_y) {
    y = caret_y - 1;
  } else {
    int unscrolled_bottom =
        GetUnscrolledY(text_area_->GetClientRect().GetBottom());
    // NOTE: Don't use (unscrolled_bottom + line_height_ / 2) / line_height_
    // Otherwise the line might be partially visible.
    int line_end = (unscrolled_bottom) / line_height_;
    if (caret_y > line_end) {
      y = view_start_y + (caret_y - line_end);
    }
  }

  int x = wxDefaultCoord;

  // If wrap is on, no horizontal scroll.
  if (!options_.wrap) {
    const int kScrollRateX = 3;  // 3 units per scroll.
    if (point.x <= view_start_x) {
      x = point.x - kScrollRateX;
      if (x < 0) {
        x = 0;
      }
    } else {
      int unscrolled_right =
          GetUnscrolledX(text_area_->GetClientRect().GetRight());
      int char_end = (unscrolled_right + char_width_ / 2) / char_width_;
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
  if (ln < 1) {
    ln = 1;
  } else if (ln > buffer_->LineCount()) {
    ln = buffer_->LineCount();
  }

  LineRange line_range = GetClientLineRange();

  if (ln < line_range.first()) {
    ScrollLinesDirectly(ln - line_range.first() - GetHalfPageSize());
  }
  if (ln > line_range.last()) {
    ScrollLinesDirectly(ln - line_range.last() + GetHalfPageSize());
  }

  TextPoint point(std::min(max_caret_x_, buffer_->LineLength(ln)), ln);
  UpdateCaretPoint(point, true, false, false);
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

//------------------------------------------------------------------------------

void TextWindow::UpdateCaretPoint(const TextPoint& point,
                                  bool line_step,
                                  bool scroll,
                                  bool vspace) {
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

  // Refresh the highlight of the caret line.
  if (caret_point_.y != p.y) {
    RefreshLineNrByLine(caret_point_.y);  // Erase
    caret_point_ = p;
    RefreshLineNrByLine(caret_point_.y);
  } else {
    caret_point_ = p;
  }

  if (!line_step) {
    max_caret_x_ = p.x;
  }

  UpdateCaretPosition();

  if (scroll) {
    ScrollToPoint(caret_point_);
  }

  // Notify the caret change.
  PostEvent(TextWindow::kCaretEvent);
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

FtPlugin* TextWindow::ft_plugin() const {
  return buffer_->ft_plugin();
}

const Options& TextWindow::options() const {
  return buffer_->options();
}

//------------------------------------------------------------------------------
// Wrap

WrapHelper* TextWindow::wrap_helper() const {
  if (wrap_helper_ == NULL) {
    wrap_helper_ = new WrapHelper(buffer_, text_extent_);
    wrap_helper_->set_client_width(text_area_->GetClientSize().GetWidth());
  }
  return wrap_helper_;
}

//------------------------------------------------------------------------------
// Handlers for buffer and buffer line changes.

void TextWindow::HandleLineUpdated(const LineChangeData& data) {
  if (!options_.wrap) {
    // Update text size and virtual size of areas, etc.
    // NOTE: Virtual height is actually not changed.
    HandleTextChange();

    RefreshTextByLineRange(data, true);
  } else {
    int wrap_delta = 0;
    int client_width = text_area_->GetClientSize().GetWidth();
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
  if (!options_.wrap) {
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
    int client_width = text_area_->GetClientSize().GetWidth();
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
  if (options_.wrap) {
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
  if (!options_.wrap) {
    RefreshLineNrAfterLine(buffer_->LineCount(), true);
  } else {
    // For simplicity, just call Refresh().
    line_nr_area_->Refresh();
  }
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

void TextWindow::OnSetFocus(wxFocusEvent& evt) {
  // Always let text area has the focus.
  if (text_area_ != NULL) {
    text_area_->SetFocus();
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
    // NOTE: Allow virtual spaces. Some actions need this, e.g.,
    // IncreaseIndentAction.
    UpdateCaretPoint(action->CaretPointAfterExec(), false, true, true);
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
    // NOTE: Allow virtual spaces.
    UpdateCaretPoint(action->caret_point(), false, true, true);
  }
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
      UpdateCaretPoint(p, false, true, true);
    } else {
      AutoIndent(p.y);
    }

    return;
  }

  // Re-indent current line if necessary.
  const TextLine* line = buffer_->Line(caret_point_.y);

  // Check if the word (or words, a indent key could be multiple words, e.g.,
  // "End If" in VB) before the caret is an indent key or not.
  if (!line->IsEmpty(true)) {
    Coord off = line->FirstNonSpaceChar();
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
  if (text_area_->GetClientSize().x < kUnreadyWindowSize) {
    return;
  }

  if (!options_.wrap) {
    UpdateVirtualSize();
    // Don't call Refresh() here!
  } else {
    wrap_helper()->set_client_width(text_area_->GetClientSize().GetWidth());

    int wrap_delta = 0;
    bool wrap_changed = wrap_helper()->Wrap(&wrap_delta);

    UpdateVirtualSize();

    if (wrap_changed) {
      // The text has to be repainted since the wrap changes.
      text_area_->Refresh();
      if (wrap_delta != 0) {
        line_nr_area_->Refresh();
      }

      // Caret position might change due to the wrap change.
      UpdateCaretPosition();
      ScrollToPoint(caret_point_);
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
  if (!options_.wrap) {
    HandleTextPaint(renderer);
  } else {
    HandleWrappedTextPaint(renderer);
  }
}

void TextWindow::HandleTextPaint(Renderer& renderer) {
  assert(!options_.wrap);

  // Get the lines to paint.
  wxRect rect = text_area_->GetUpdateClientRect();
  LineRange line_range(LineFromScrolledY(rect.GetTop()),
                       LineFromScrolledY(rect.GetBottom()));
  if (line_range.IsEmpty()) {
    return;  // No lines to paint.
  }

  const wxRect client_rect = text_area_->GetClientRect();
  int x = client_rect.GetLeft();

  // NOTE: Text background is not supported.
  renderer.SetTextBackground(wxNullColour);

  const Coord line_count = buffer_->LineCount();
  const int line_padding = (line_height_ - char_height_) / 2;

  int y1 = client_rect.y +
           line_height_ * (line_range.first() - 1) +
           line_padding;
  int y2 = y1;

  Coord ln = line_range.first();
  for (; ln <= line_range.last() && ln <= line_count; ++ln) {
    DrawTextLine(ln, renderer, x, y2);
  }

  // Blank lines.
  if (ln <= line_range.last()) {
    const wxColour& blank_bg = style_->Get(Style::kBlank)->bg();

    if (blank_bg != style_->Get(Style::kNormal)->bg()) {
      renderer.SetPen(wxPen(blank_bg), true);
      renderer.SetBrush(wxBrush(blank_bg), true);

      int blank_w = text_area_->GetVirtualSize().x;
      int blank_h = line_height_ * (line_range.last() - ln + 1);
      renderer.DrawRectangle(x, y2, blank_w, blank_h);

      y2 += blank_h;

      renderer.RestoreBrush();
      renderer.RestorePen();
    }
  }

  // Rulers.
  if (!options_.rulers.empty()) {
    renderer.SetPen(wxPen(theme_->GetColor(RULER)), true);

    for (size_t i = 0; i < options_.rulers.size(); ++i) {
      int ruler_x = x + char_width_ * options_.rulers[i];
      renderer.DrawLine(ruler_x, y1, ruler_x, y2);
    }

    renderer.RestorePen();
  }
}

void TextWindow::HandleWrappedTextPaint(Renderer& renderer) {
  assert(options_.wrap);

  // Get the lines to paint.
  wxRect rect = text_area_->GetUpdateClientRect();
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

  // Unwrap the wrapped line range.
  LineRange line_range;
  if (!wrapped_line_range.IsEmpty()) {
    line_range = wrap_helper()->UnwrapLineRange(wrapped_line_range);
  }

  // NOTE: Text background is not supported.
  renderer.SetTextBackground(wxNullColour);

  const wxRect client_rect = text_area_->GetClientRect();
  int x = client_rect.GetLeft();
  const int line_padding = (line_height_ - char_height_) / 2;

  if (!line_range.IsEmpty()) {
    Coord wrapped_first_ln = wrap_helper()->WrapLineNr(line_range.first());
    int y1 = client_rect.y + line_height_ * (wrapped_first_ln - 1);
    int y2 = y1 + line_padding;

    const Coord line_count = buffer_->LineCount();

    Coord ln = line_range.first();
    for (; ln <= line_range.last() && ln <= line_count; ++ln) {
      DrawWrappedTextLine(ln, renderer, x, y2);
    }

    // Rulers.
    if (!options_.rulers.empty()) {
      renderer.SetPen(wxPen(theme_->GetColor(RULER)), true);

      y2 -= line_padding;

      for (size_t i = 0; i < options_.rulers.size(); ++i) {
        int ruler_x = x + char_width_ * options_.rulers[i];
        renderer.DrawLine(ruler_x, y1, ruler_x, y2);
      }

      renderer.RestorePen();
    }
  }

  // Blank lines.
  if (!blank_line_range.IsEmpty()) {
    const wxColour& blank_bg = style_->Get(Style::kBlank)->bg();

    if (blank_bg != style_->Get(Style::kNormal)->bg()) {
      renderer.SetPen(wxPen(blank_bg), true);
      renderer.SetBrush(wxBrush(blank_bg), true);

      int y = client_rect.y + line_height_ * (blank_line_range.first() - 1);
      int w = text_area_->GetVirtualSize().x;
      int h = line_height_ * blank_line_range.LineCount();

      renderer.DrawRectangle(x, y + line_padding, w, h);

      renderer.RestoreBrush();
      renderer.RestorePen();

      // Rulers.
      if (!options_.rulers.empty()) {
        renderer.SetPen(wxPen(theme_->GetColor(RULER)), true);

        int y2 = y + h;
        for (size_t i = 0; i < options_.rulers.size(); ++i) {
          int ruler_x = x + char_width_ * options_.rulers[i];
          renderer.DrawLine(ruler_x, y, ruler_x, y2);
        }

        renderer.RestorePen();
      }
    }
  }
}

void TextWindow::DrawTextLine(Coord ln, Renderer& renderer, int x, int& y) {
  assert(!options_.wrap);

  // If in select range, draw the select background.
  if (selection_.HasLine(ln)) {
    const wxColour& visual_bg = style_->Get(Style::kVisual)->bg();
    renderer.SetBrush(wxBrush(visual_bg), true);
    renderer.SetPen(wxPen(visual_bg), true);

    CharRange char_range = selection_.GetCharRange(ln);

    if (char_range.IsEmpty()) {
      // Draw a vertical line for empty rect selection.
      int x = GetLineWidth(ln, 0, char_range.begin());
      renderer.DrawLine(x, y, x, y + line_height_);
    } else {
      int x_begin = GetLineWidth(ln, 0, char_range.begin());
      int x_end = GetLineWidth(ln, 0, char_range.end());
      int w = x_end - x_begin;
      if (ln != selection_.end().y && char_range.end() == kInvalidCoord) {
        w += char_width_;  // Extra char width for EOL.
      }

      renderer.DrawRectangle(x_begin, y, w, line_height_);
    }

    renderer.RestoreBrush();
    renderer.RestorePen();
  }

  DrawTextLine(renderer, buffer_->Line(ln), x, y);

  y += line_height_;
}

void TextWindow::DrawWrappedTextLine(Coord ln,
                                     Renderer& renderer,
                                     int x,
                                     int& y) {
  assert(options_.wrap);

  const TextLine* line = buffer_->Line(ln);

  std::vector<CharRange> sub_ranges = wrap_helper()->SubRanges(ln);

  // If in select range, draw the select background.
  if (selection_.HasLine(ln)) {
    const wxColour& visual_bg = style_->Get(Style::kVisual)->bg();
    renderer.SetBrush(wxBrush(visual_bg), true);
    renderer.SetPen(wxPen(visual_bg), true);

    CharRange select_char_range = selection_.GetCharRange(ln);
    int _y = y;

    std::vector<CharRange>::iterator range_it = sub_ranges.begin();
    for (; range_it != sub_ranges.end(); ++range_it) {
      CharRange& sub_range = *range_it;

      CharRange sub_select_char_range = sub_range.Intersect(select_char_range);
      if (sub_select_char_range.IsEmpty()) {
        _y += line_height_;
        continue;  // No intersection with the select range.
      }

      int x1 = GetLineWidth(line,
                            sub_range.begin(),
                            sub_select_char_range.begin());
      int x2 = GetLineWidth(line,
                            sub_range.begin(),
                            sub_select_char_range.end());
      int w = x2 - x1;
      if (ln != selection_.end().y &&
          sub_select_char_range.end() == kInvalidCoord) {
        w += char_width_;  // Extra char width for EOL.
      }

      renderer.DrawRectangle(x1, _y, w, line_height_);

      _y += line_height_;
    }

    renderer.RestorePen();
    renderer.RestoreBrush();
  }

  const std::wstring& line_data = line->data();

  if (ft_plugin()->IsLexAvailable()) {
    // For calculating spaces occupied by a tab.
    Coord chars = 0;

    std::vector<CharRange>::iterator range_it = sub_ranges.begin();
    for (; range_it != sub_ranges.end(); ++range_it) {
      CharRange& sub_range = *range_it;

      if (sub_range.end() == kInvalidCoord) {  // Last sub range
        sub_range.set_end(line_data.size());
      }

      int _x = x;
      int _y = y;

      std::list<const LexElement*> lex_elements = line->lex_elements(sub_range);

      if (lex_elements.empty()) {
        // Without lex elements.

        // Get the range, [i, j), of the line piece to draw.
        Coord i = sub_range.begin();
        Coord j = sub_range.end();
        DrawTextLinePiece(renderer, line_data, i, j, Lex(), _x, _y, chars);

        y += line_height_;

        continue;
      }

      // With lex elements.

      Coord i = sub_range.begin();

      std::list<const LexElement*>::iterator le_it = lex_elements.begin();
      for (; le_it != lex_elements.end(); ++le_it) {
        const LexElement* le = *le_it;

        // Draw the line piece before the lex element.
        if (i < le->off) {
          Coord j = le->off;
          DrawTextLinePiece(renderer, line_data, i, j, Lex(), _x, _y, chars);
          i = j;
        }

        // Draw the line piece of the lex element.
        Coord j = le->off + le->len;
        if (j > sub_range.end()) {
          j = sub_range.end();
        }

        DrawTextLinePiece(renderer, line_data, i, j, le->lex, _x, _y, chars);
        i = j;
      }

      // Draw the line piece after the last lex element.
      if (i < sub_range.end()) {
        // Line piece (spaces, operators, plain-text, etc.) with no lex element.
        Coord j = sub_range.end();
        DrawTextLinePiece(renderer, line_data, i, j, Lex(), _x, _y, chars);
      }

      y += line_height_;
    }
  } else {
    // For calculating spaces occupied by a tab.
    Coord chars = 0;

    std::vector<CharRange>::iterator range_it = sub_ranges.begin();
    for (; range_it != sub_ranges.end(); ++range_it) {
      CharRange& sub_range = *range_it;

      if (sub_range.end() == kInvalidCoord) {  // Last sub range
        sub_range.set_end(line_data.size());
      }

      int _x = x;
      int _y = y;

      // Get the range, [i, j), of the line piece to draw.
      Coord i = sub_range.begin();
      Coord j = sub_range.end();
      DrawTextLinePiece(renderer, line_data, i, j, Lex(), _x, _y, chars);

      y += line_height_;
    }
  }
}

void TextWindow::DrawTextLine(Renderer& renderer,
                              const TextLine* line,
                              int x,
                              int y) {
  const std::wstring& line_data = line->data();
  if (line_data.empty()) {
    return;
  }

  // For calculating spaces occupied by a tab.
  Coord chars = 0;

  if (!ft_plugin()->IsLexAvailable()) {
    Coord j = line->Length();
    DrawTextLinePiece(renderer, line_data, 0, j, Lex(), x, y, chars);
    return;
  }

  int _x = x;
  int _y = y;

#if JIL_TEST_UNDERLINE_LEX_ELEMENT
  renderer.SetPen(*wxRED_PEN);
#endif

  const std::list<LexElement*>& lex_elements = line->lex_elements();

  Coord i = 0;
  Coord j = 0;

  std::list<LexElement*>::const_iterator le_it = lex_elements.begin();
  for (; le_it != lex_elements.end(); ++le_it) {
    const LexElement* le = *le_it;

    if (i < le->off) {
      // Line piece (spaces, operators, plain-text, etc.) with no lex.
      DrawTextLinePiece(renderer, line_data, i, le->off, Lex(), _x, _y, chars);
    }

#if JIL_TEST_UNDERLINE_LEX_ELEMENT
    int ul_x1 = _x + 3;
    int ul_y = _y + char_height_;
#endif

    i = le->off;
    j = i + le->len;
    DrawTextLinePiece(renderer, line_data, i, j, le->lex, _x, _y, chars);

#if JIL_TEST_UNDERLINE_LEX_ELEMENT
    int ul_x2 = _x - 3;
    renderer.DrawLine(ul_x1, ul_y, ul_x2, ul_y);
#endif

    i = j;
  }

#if JIL_TEST_UNDERLINE_LEX_ELEMENT
  renderer.RestorePen();
#endif

  // Draw line piece after the last lex element.
  j = line->Length();
  if (i < j) {
    // Line piece (spaces, operators, plain-text, etc.) with no lex element.
    DrawTextLinePiece(renderer, line_data, i, j, Lex(), _x, _y, chars);
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
  wxPen space_pen(style_->Get(Style::kSpace)->fg());

  const StyleValue* lex_style_value = style_->Get(lex);
  SetRendererStyle(renderer, lex_style_value);

  Coord p = i;

  for (; i < j; ++i) {
    if (line_data[i] != kSpaceChar &&
        line_data[i] != kTabChar &&
        !ft_plugin()->IsOperator(line_data[i])) {
      ++chars;
      continue;
    }

    // Render the line piece before this space, tab or operator.
    if (i > p) {
      DrawTextWord(renderer, line_data, p, i - p, lex, x, y, chars);
    }

    // Render the space(s) or tab(s).
    if (line_data[i] == kSpaceChar) {
      Coord spaces = CountCharAfter(line_data, i, kSpaceChar) + 1;
      if (options_.show_space) {
        renderer.SetPen(space_pen);
        renderer.DrawWhiteSpaces(x, y, spaces);
      }
      x += char_width_ * spaces;
      chars += spaces;
      i += spaces - 1;
      p = i + 1;
    } else if (line_data[i] == kTabChar) {
      // The tab might occupy "< tab_stop" spaces.
      int tab_spaces = options_.tab_stop - (chars % options_.tab_stop);
      chars += tab_spaces;

      int tab_w = char_width_ * tab_spaces;
      if (options_.show_space) {
        renderer.SetPen(space_pen);
        renderer.DrawTab(x, y, tab_w, char_height_);
      }
      x += tab_w;

      p = i + 1;
    } else {  // Operator
      // Count operators.
      Coord operators = 1;
      for (Coord k = i + 1;
           k < j && ft_plugin()->IsOperator(line_data[k]);
           ++k) {
        ++operators;
      }

      if (lex.major() == kLexNone) {
        SetRendererStyle(renderer, style_->Get(Style::kOperator));
      } else {
        SetRendererStyle(renderer, lex_style_value);
      }

      int piece_w = 0;
      renderer.DrawText(line_data, i, operators, x, y, &piece_w);
      x += piece_w;

      if (lex.major() == kLexNone) {
        // Restore style.
        SetRendererStyle(renderer, lex_style_value);
      }

      chars += operators;
      i += operators - 1;
      p = i + 1;
    }
  }

  // Render the last line piece if any.
  if (p < j) {
    DrawTextWord(renderer, line_data, p, j - p, lex, x, y, chars);
  }
}

void TextWindow::DrawTextWord(Renderer& renderer,
                              const std::wstring& line_data,
                              Coord off,
                              Coord len,
                              Lex lex,
                              int& x,
                              int y,
                              Coord& chars) {
  SetRendererStyle(renderer, style_->Get(lex));

  int piece_w = 0;
  renderer.DrawText(line_data, off, len, x, y, &piece_w);
  x += piece_w;
}

void TextWindow::SetRendererStyle(Renderer& renderer,
                                  const StyleValue* style_value) {
  if (style_value != NULL) {
    wxFont font = text_area_->GetFont();

    if ((style_value->font() & Style::kBold) != 0) {
      font.SetWeight(wxFONTWEIGHT_BOLD);
    }
    if ((style_value->font() & Style::kItalic) != 0) {
      font.SetStyle(wxFONTSTYLE_ITALIC);
    }

    if (style_value->fg().IsOk()) {
      renderer.SetFont(font, style_value->fg());
    } else {
      renderer.SetFont(font, style_->Get(Style::kNormal)->fg());
    }
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
    TextRange bracket_pair_range;

    if (buffer_->IsBracketPairInnerRange(selection_.range)) {
      // If it's already inner range, increase to outer range.
      // TODO: Return empty range if not increased.
      bracket_pair_range = buffer_->IncreaseRange(selection_.range);
    } else {
      // Get inner range.
      bracket_pair_range = buffer_->BracketPairInnerRange(down_point_);
    }

    if (!bracket_pair_range.IsEmpty()) {
      SetSelection(bracket_pair_range.point_begin(),
                   bracket_pair_range.point_end(),
                   false);
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
    // Select by rectangle.
    // TODO
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
    scroll_timer_ = new wxTimer(this, wxID_ANY);
    Connect(scroll_timer_->GetId(),
            wxEVT_TIMER,
            wxTimerEventHandler(TextWindow::OnScrollTimer));
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
    Disconnect(wxEVT_TIMER, scroll_timer_->GetId());
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

void TextWindow::HandleTextLeftDClick(wxMouseEvent& evt) {
  // Double-click selects the clicked word.
  TextPoint point = CalcCaretPoint(evt.GetPosition(), false);
  SelectByWord(point, point);
}

bool TextWindow::HandleTextMouseWheel(wxMouseEvent& evt) {
  if (evt.ControlDown()) {
    // Increase/decrease font size.

    wxFont font = text_area_->GetFont();
    int point_size = font.GetPointSize();

    if (evt.GetWheelRotation() > 0) {
      ++point_size;
    } else if (evt.GetWheelRotation() < 0) {
      --point_size;
    }

    const int kMinFontSize = 8;
    if (point_size >= kMinFontSize) {
      font.SetPointSize(point_size);

      SetTextFont(font);
      SetLineNrFont(font);
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

void TextWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
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

  if (code == WXK_TAB && modifiers == 0 && leader_key_->IsEmpty()) {
    // Input tab (expand or not).
    if (options_.expand_tab) {
      int spaces = options_.tab_stop - (caret_point_.x % options_.tab_stop);
      // TODO: Continuous tabs cannot be undone together.
      InsertString(std::wstring(spaces, kSpaceChar));
    } else {
      InsertChar(kTabChar);
    }
    return true;
  }

  // Avoid matching for single character key.
  if (leader_key_->IsEmpty()) {
    if (modifiers == 0) {
      // Standard ASCII characters || ASCII extended characters.
      if (code >= 33 && code <= 126 || code >= 128 && code <= 255) {
        return false;
      }
    } else if (modifiers == wxMOD_SHIFT && code < 127) {
      if (editor::kNonShiftChars.find(static_cast<char>(code))
          != std::string::npos) {
        // ~!@#$%^&*()_+<>?:"{}|
        return false;
      }
    }
  }

  editor::Key key(code, modifiers);
  if (!leader_key_->IsEmpty()) {
    key.set_leader(*leader_key_);
  }

  // Get text function by key.
  TextFunc* text_func = binding_->GetTextFuncByKey(key, mode());

  // Reset leader key no matter if the text function is found or not.
  if (!leader_key_->IsEmpty()) {
    leader_key_->Reset();
    PostEvent(TextWindow::kLeaderKeyEvent);
  }

  if (text_func != NULL) {
    text_func->Exec(this);
    return true;
  }

  return false;
}

void TextWindow::OnTextChar(wxKeyEvent& evt) {
  if (evt.AltDown() || evt.CmdDown()) {
    evt.Skip();
    return;
  }

  wchar_t c = evt.GetUnicodeKey();
  if (c >= 0x20) {  // "< 0x20" is CONTROL
    InsertChar(c);
  }
}

//------------------------------------------------------------------------------
// Delegated event handlers from LineNrArea.

// Paint the line number area.
void TextWindow::OnLineNrPaint(wxDC& dc) {
  if (!options_.wrap) {
    HandleLineNrPaint(dc);
  } else {
    HandleWrappedLineNrPaint(dc);
  }
}

void TextWindow::HandleLineNrPaint(wxDC& dc) {
  assert(!options_.wrap);

  // Get the lines to paint.
  wxRect rect = line_nr_area_->GetUpdateClientRect();
  LineRange line_range(LineFromScrolledY(rect.GetTop()),
                       LineFromScrolledY(rect.GetBottom()));
  if (line_range.IsEmpty()) {
    return;  // No lines to paint.
  }

  const Coord line_count = buffer_->LineCount();

  LineRange blank_line_range;

  if (line_range.first() > line_count) {
    blank_line_range = line_range;
    line_range.Set(0);  // No text to draw.
  } else {
    if (line_range.last() > line_count) {
      blank_line_range.Set(line_count + 1, line_range.last());
      line_range.set_last(line_count);  // Remove the blank lines.
    }
  }

  const wxRect client_rect = line_nr_area_->GetClientRect();
  int x = client_rect.GetLeft() + GetUnscrolledX(0);

  if (!line_range.IsEmpty()) {
    // Caret line highlight.
    if (line_range.Has(caret_point_.y)) {
      const wxColour& bg = style_->Get(Style::kCaretLine)->bg();
      dc.SetPen(wxPen(bg));
      dc.SetBrush(wxBrush(bg));

      int y = client_rect.GetTop() + line_height_ * (caret_point_.y - 1);
      int w = client_rect.GetWidth() - kLineNrHlPaddingRight;

      dc.DrawRectangle(x, y, w, char_height_);
    }

    // Draw right aligned line numbers.
    if (options_.show_number) {
      dc.SetFont(line_nr_area_->GetFont());
      dc.SetTextForeground(style_->Get(Style::kNumber)->fg());

      int y = client_rect.y + line_height_ * (line_range.first() - 1);
      int w = client_rect.width - kLineNrPaddingRight;
      wxRect rect(x, y, w, line_height_);

      wxString label;
      for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
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
    if (!options_.show_number || line_range.IsEmpty()) {
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
  assert(options_.wrap);

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
      int h = char_height_ * wrap_helper()->WrappedLineCount(caret_point_.y);

      dc.DrawRectangle(x, y, w, h);
    }

    // Draw right aligned line numbers.
    if (options_.show_number) {
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
    if (!options_.show_number || wrapped_line_range.IsEmpty()) {
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

  if (!options_.wrap) {
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
    caret_point.x = GetWrappedCharIndex(caret_point.y,
                                        sub_ln,
                                        unscrolled_pos.x,
                                        vspace);
  }

  return caret_point;
}

wxRect TextWindow::ClientRectFromLineRange(wxWindow* area,
                                           const LineRange& line_range) const {
  LineRange line_range_copy = line_range;
  if (options_.wrap) {
    line_range_copy = wrap_helper()->WrapLineRange(line_range_copy);
  }

  int scrolled_y = GetScrolledY(line_height_ * (line_range_copy.first() - 1));

  const wxRect client_rect = area->GetClientRect();
  return wxRect(0,
                scrolled_y,
                client_rect.GetWidth(),
                line_height_ * line_range_copy.LineCount());
}

wxRect TextWindow::ClientRectAfterLine(wxWindow* area,
                                       Coord ln,
                                       bool included) const {
  Coord v_ln = ln;
  if (options_.wrap) {
    v_ln = wrap_helper()->WrapLineNr(ln);
  }

  if (!included) {
    if (!options_.wrap) {
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

// NOTE: @ln might be invalid.
// E.g., called by UpdateCaretPoint() to erase the previous caret line
// which has been deleted.
void TextWindow::RefreshTextByLine(Coord ln, bool update) {
  if (ln <= buffer_->LineCount()) {
    RefreshTextByLineRange(LineRange(ln), update);
  }
}

void TextWindow::RefreshTextByLineRange(const LineRange& line_range,
                                        bool update) {
  wxRect refresh_rect = ClientRectFromLineRange(text_area_, line_range);
  text_area_->RefreshRect(refresh_rect);
  if (update) {
    text_area_->Update();
  }
}

// NOTE: @ln might be invalid.
// E.g., called by UpdateCaretPoint() to erase the previous caret line
// which has been deleted.
void TextWindow::RefreshLineNrByLine(Coord ln, bool update) {
  if (ln <= buffer_->LineCount()) {
    RefreshLineNrByLineRange(LineRange(ln), update);
  }
}

void TextWindow::RefreshLineNrByLineRange(const LineRange& line_range,
                                          bool update) {
  wxRect rect = ClientRectFromLineRange(line_nr_area_, line_range);
  line_nr_area_->RefreshRect(rect);
  if (update) {
    line_nr_area_->Update();
  }
}

//------------------------------------------------------------------------------

void TextWindow::HandleTextFontChange() {
  // Update the font of text extent.
  text_extent_->SetFont(text_area_->GetFont());

  UpdateCharSize();
  SetScrollbars(char_width_, line_height_, 1, 1);

  UpdateTextSize();
  UpdateVirtualSize();

  // Update caret height.
  text_area_->GetCaret()->SetSize(kCaretWidth, line_height_);
}

void TextWindow::HandleLineNrFontChange() {
  int old_line_nr_width = line_nr_width_;
  UpdateLineNrWidth();

  // If the line number width changes, layout the areas.
  if (old_line_nr_width != line_nr_width_) {
    LayoutAreas();
  }
}

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

void TextWindow::UpdateCharSize() {
  int ext_leading = 0;  // Usually 0.
  text_extent_->GetExtent(L"T", &char_width_, &char_height_, &ext_leading);
  line_height_ = char_height_ + ext_leading + kLinePaddingY;
}

void TextWindow::UpdateTextSize() {
  if (!options_.wrap) {
    text_width_ = char_width_ * buffer_->GetMaxLineLength();
    text_height_ = line_height_ * buffer_->LineCount();
  } else {
    text_width_ = -1;
    text_height_ = line_height_ * wrap_helper()->WrappedLineCount();
  }
}

void TextWindow::UpdateLineNrWidth() {
  if (options_.show_number) {
    wxString line_nr_str = wxString::Format(L"%d", buffer_->LineCount());
    line_nr_width_ = text_extent_->GetWidth(line_nr_str.wc_str());
    line_nr_width_ += kLineNrPadding;
  } else {
    int tilde_width = text_extent_->GetWidth(kTilde.wc_str());
    line_nr_width_ = tilde_width + kLineNrPadding;
  }
}

void TextWindow::UpdateVirtualSize() {
  Coord line_count = 0;
  if (!options_.wrap) {
    line_count = buffer_->LineCount();
  } else {
    line_count = wrap_helper()->WrappedLineCount();
  }

  // -1 to keep the last line visible when scroll to the end.
  int vh = line_height_ * (line_count + GetPageSize() - 1);

  if (!options_.wrap) {
    // - char_width_ to keep the last char visible when scroll to the end.
    int vw = text_width_ + text_area_->GetClientSize().GetWidth() - char_width_;
    text_area_->SetVirtualSize(vw, vh);
  } else {
    text_area_->SetVirtualSize(-1, vh);
  }

  line_nr_area_->SetVirtualSize(-1, vh);

  AdjustScrollbars();
}

void TextWindow::LayoutAreas() {
  const wxRect client_rect = GetClientRect();

  int text_area_height = client_rect.GetHeight() -
                         kMarginTop -
                         kMarginBottom -
                         kTextMarginBottom;

  line_nr_area_->SetSize(0, kMarginTop, line_nr_width_, text_area_height);

  text_area_->SetSize(line_nr_width_ + kTextMarginLeft,
                      kMarginTop,
                      client_rect.GetWidth() - kTextMarginLeft - line_nr_width_,
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

  if (!options_.wrap) {
    y = caret_point_.y;
  } else {
    Coord sub_ln = wrap_helper()->SubLineNr(caret_point_.y,
                                            caret_point_.x,
                                            &x_off);
    // -1 because sub_ln is 1-based.
    y = wrap_helper()->WrapLineNr(caret_point_.y) + sub_ln - 1;
  }

  int unscrolled_x = GetLineWidth(caret_point_.y, x_off, caret_point_.x);
  int unscrolled_y = (y - 1) * line_height_;

  wxPoint p;
  CalcScrolledPosition(unscrolled_x, unscrolled_y, &p.x, &p.y);

  text_area_->GetCaret()->Move(p);
}

//------------------------------------------------------------------------------
// Selection

void TextWindow::SetSelection(const TextPoint& point_from,
                              const TextPoint& point_to,
                              bool vspace) {
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

void TextWindow::SelectByWord(const TextPoint& point_from,
                              const TextPoint& point_to) {
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

int TextWindow::GetLineWidth(const TextLine* line,
                             Coord off1,
                             Coord off2) const {
  assert(off1 <= line->Length());

  int line_width = 0;

  if (off2 == kInvalidCoord) {
    off2 = line->Length();
  } else if (off2 > line->Length()) {
    line_width = char_width_ * (off2 - line->Length());
    off2 = line->Length();
  }

  std::wstring line_data = line->data().substr(off1, off2 - off1);
  if (!line_data.empty()) {
    TabbedLineFast(options_.tab_stop, &line_data);
    line_width += text_extent_->GetWidth(line_data);
  }

  return line_width;
}

int TextWindow::GetLineWidth(Coord ln, Coord off1, Coord off2) const {
  return GetLineWidth(buffer_->Line(ln), off1, off2);
}

Coord TextWindow::GetCharIndex(Coord ln, int client_x, bool vspace) const {
  return text_extent_->IndexChar(options_.tab_stop,
                                 buffer_->LineData(ln),
                                 client_x,
                                 vspace);
}

Coord TextWindow::GetWrappedCharIndex(Coord ln,
                                      Coord wrapped_sub_ln,
                                      int client_x,
                                      bool vspace) const {
  assert(wrapped_sub_ln >= 1);

  if (wrapped_sub_ln == 1) {
    return GetCharIndex(ln, client_x, vspace);
  } else {  // >= 2
    const WrapInfo& wrap_info = wrap_helper()->GetWrapInfo(ln);
    Coord offset = wrap_info.offsets()[wrapped_sub_ln - 2];

    // TODO: Avoid copy.
    std::wstring line_data = buffer_->LineData(ln).substr(offset);
    Coord i = text_extent_->IndexChar(options_.tab_stop,
                                      line_data,
                                      client_x,
                                      vspace);
    return offset + i;
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

void TextWindow::PostEvent(TextWindow::EventType type) {
  wxCommandEvent evt(kTextWindowEvent, GetId());
  evt.SetEventObject(this);
  evt.SetInt(type);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace editor
}  // namespace jil
