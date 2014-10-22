#include "editor/action.h"
#include <algorithm>
#include <utility>
#include "editor/text_buffer.h"
#include "editor/ft_plugin.h"

namespace jil {
namespace editor {

Action::Action(TextBuffer* buffer, const TextPoint& point)
    : saved_(false)
    , buffer_(buffer)
    , timestamp_(wxDateTime::UNow())
    , point_(point)
    , delta_point_(0, 0)
    , caret_point_(point)
    , update_caret_(true)
    , effective_(true)
    , grouped_(false) {
}

Action::Action(const Action& rhs)
    : saved_(false)
    , buffer_(rhs.buffer_)
    , timestamp_(wxDateTime::UNow())
    , point_(rhs.point_)
    , delta_point_(0, 0)
    , caret_point_(rhs.caret_point_)
    , effective_(true)
    , grouped_(false) {
}

Action::~Action() {
}

////////////////////////////////////////////////////////////////////////////////

GroupAction::GroupAction(TextBuffer* buffer)
    : Action(buffer, TextPoint()) {
  update_caret_ = false;
  effective_ = true;
  grouped_ = true;
}

GroupAction::~GroupAction() {
}

void GroupAction::Exec() {
}

void GroupAction::Undo() {
}

////////////////////////////////////////////////////////////////////////////////

InsertCharAction::InsertCharAction(TextBuffer* buffer,
                                   const TextPoint& point,
                                   wchar_t c)
    : Action(buffer, point), dir_(kForward), c_(c) {
}

InsertCharAction::~InsertCharAction() {
}

void InsertCharAction::Exec() {
  if (dir_ == kForward) {
    if (c_ == LF) {
      delta_point_.x = -point_.x;
      delta_point_.y = 1;
    } else {
      delta_point_.x = 1;
    }
  }

  buffer_->InsertChar(point_, c_);
}

void InsertCharAction::Undo() {
  buffer_->DeleteChar(point_);
}

////////////////////////////////////////////////////////////////////////////////

InsertStringAction::InsertStringAction(TextBuffer* buffer,
                                       const TextPoint& point,
                                       const std::wstring& str)
    : Action(buffer, point), str_(str) {
}

InsertStringAction::~InsertStringAction() {
}

void InsertStringAction::Exec() {
  buffer_->InsertString(point_, str_);
  delta_point_.x = CoordCast(str_.size());
}

void InsertStringAction::Undo() {
  buffer_->DeleteString(point_, CoordCast(str_.size()));
}

////////////////////////////////////////////////////////////////////////////////

InsertTextAction::InsertTextAction(TextBuffer* buffer,
                                   const TextPoint& point,
                                   const std::wstring& text)
    : Action(buffer, point)
    , text_(text)
    , use_delta_x_(true)
    , use_delta_y_(true) {
}

InsertTextAction::~InsertTextAction() {
}

void InsertTextAction::Exec() {
  TextPoint last_point = buffer_->InsertText(point_, text_);
  delta_point_ = last_point - point_;
}

void InsertTextAction::Undo() {
  buffer_->DeleteText(TextRange(point_, point_ + delta_point_), NULL);
}

TextPoint InsertTextAction::CaretPointAfterExec() const {
  TextPoint caret_point = caret_point_;

  if (use_delta_x_) {
    caret_point.x += delta_point_.x;
  }
  if (use_delta_y_) {
    caret_point.y += delta_point_.y;
  }

  return caret_point;
}

////////////////////////////////////////////////////////////////////////////////

DeleteAction::DeleteAction(TextBuffer* buffer,
                           const TextPoint& point,
                           TextUnit text_unit,
                           SeekType seek_type)
    : Action(buffer, point)
    , seek_type_(seek_type)
    , text_unit_(text_unit)
    , count_(1) {
}

DeleteAction::~DeleteAction() {
}

void DeleteAction::Exec() {
  bool by_line = (text_unit_ == kLine && seek_type_ == kWhole);

  TextPoint another_point;
  if (by_line) {
    another_point = Seek(point_, text_unit_, kNext, count_);
  } else {
    another_point = Seek(point_, text_unit_, seek_type_, count_);
  }

  if (another_point < point_) {
    DeleteText(TextRange(another_point, point_));
    delta_point_ = another_point - point_;
  } else if (another_point > point_) {
    DeleteText(TextRange(point_, another_point));
  } else {  // Nothing to delete.
    effective_ = false;
  }
}

void DeleteAction::DeleteText(const TextRange& range) {
  if (!text_.empty()) {  // Ever executed.
    buffer_->DeleteText(range);
  } else {
    buffer_->DeleteText(range, &text_);
  }
}

TextPoint DeleteAction::CaretPointAfterExec() const {
  if (text_unit_ == kLine && seek_type_ == kWhole) {
    return caret_point_;
  }

  return point_ + delta_point_;
}

void DeleteAction::Undo() {
  if (!text_.empty()) {
    buffer_->InsertText(point_ + delta_point_, text_);
  }
}

bool DeleteAction::SameKind(const DeleteAction& rhs) const {
  return (text_unit_ == rhs.text_unit_ &&
          seek_type_ == rhs.seek_type_ &&
          saved_ == rhs.saved_);
}

bool DeleteAction::Merge(DeleteAction* next_delete_action) {
  assert(SameKind(*next_delete_action));

  bool merged = false;

  if (!next_delete_action->effective()) {
    return merged;
  }

  if (text_unit_ == kChar || text_unit_ == kWord) {
    if (seek_type_ == kPrev) {
      text_ = next_delete_action->text_ + text_;
      merged = true;
    } else if (seek_type_ == kNext) {
      text_ += next_delete_action->text_;
      merged = true;
    }
  }

  if (merged) {
    count_ += next_delete_action->count_;
    delta_point_ += next_delete_action->delta_point_;
    timestamp_ = next_delete_action->timestamp_;
  }

  return merged;
}

TextPoint DeleteAction::Seek(const TextPoint& point,
                             TextUnit text_unit,
                             SeekType seek_type,
                             size_t count) const {
  TextPoint seek_point = point;
  for (size_t c = 0; c < count; ++c) {
    seek_point = buffer_->Seek(seek_point, text_unit, seek_type);
  }
  return seek_point;
}

////////////////////////////////////////////////////////////////////////////////

DeleteRangeAction::DeleteRangeAction(TextBuffer* buffer,
                                     const TextRange& range,
                                     TextDir dir,
                                     bool rect,
                                     bool selected)
    : Action(buffer, dir == kForward ? range.point_end() : range.point_begin())
    , RangeAction(range, dir, rect, selected) {
}

DeleteRangeAction::~DeleteRangeAction() {
}

void DeleteRangeAction::Exec() {
  if (range_.IsEmpty()) {
    return;
  }

  if (!text_.empty()) {  // Ever executed.
    buffer_->DeleteText(range_);
  } else {
    buffer_->DeleteText(range_, &text_);
  }

  if (dir_ == kForward) {
    delta_point_ = range_.point_begin() - range_.point_end();
  }
}

void DeleteRangeAction::Undo() {
  if (!text_.empty()) {
    buffer_->InsertText(range_.point_begin(), text_);
  }
}

////////////////////////////////////////////////////////////////////////////////

IncreaseIndentAction::IncreaseIndentAction(TextBuffer* buffer,
                                           const TextRange& range,
                                           TextDir dir,
                                           bool rect,
                                           bool selected)
    : Action(buffer, TextPoint())
    , RangeAction(range, dir, rect, selected) {
}

IncreaseIndentAction::~IncreaseIndentAction() {
}

void IncreaseIndentAction::Exec() {
  const Options& options = buffer_->ft_plugin()->options();

  if (options.expand_tab) {
    spaces_ = std::wstring(options.tab_stop, kSpaceChar);
  } else {
    spaces_ = std::wstring(1, kTabChar);
  }

  delta_point_.Set(CoordCast(spaces_.size()), 0);

  LineRange line_range = range_.GetLineRange();
  if (line_range.LineCount() > 1) {
    buffer_->FreezeNotify();
  }

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    if (!buffer_->IsLineEmpty(ln, true)) {  // Skip empty lines.
      buffer_->InsertString(TextPoint(0, ln), spaces_);
    }
  }

  if (line_range.LineCount() > 1) {
    buffer_->ThawNotify();
    buffer_->Notify(kLineUpdated, line_range);
  }
}

void IncreaseIndentAction::Undo() {
  LineRange line_range = range_.GetLineRange();

  if (line_range.LineCount() > 1) {
    buffer_->FreezeNotify();
  }

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    if (!buffer_->IsLineEmpty(ln, true)) {  // Skip empty lines.
      buffer_->DeleteString(TextPoint(0, ln), CoordCast(spaces_.size()));
    }
  }

  if (line_range.LineCount() > 1) {
    buffer_->ThawNotify();
    buffer_->Notify(kLineUpdated, line_range);
  }
}

TextRange IncreaseIndentAction::SelectionAfterExec() const {
  if (range_.IsEmpty()) {
    return range_;
  }

  const Options& options = buffer_->ft_plugin()->options();
  Coord spaces = options.expand_tab ? options.tab_stop : 1;

  TextPoint point_begin(range_.point_begin());
  if (!buffer_->IsLineEmpty(point_begin.y, true)) {
    point_begin.x += spaces;
  }

  TextPoint point_end(range_.point_end());
  if (!buffer_->IsLineEmpty(point_end.y, true)) {
    point_end.x += spaces;
  }

  return TextRange(point_begin, point_end);
}

////////////////////////////////////////////////////////////////////////////////

DecreaseIndentAction::DecreaseIndentAction(TextBuffer* buffer,
                                           const TextRange& range,
                                           TextDir dir,
                                           bool rect,
                                           bool selected)
    : Action(buffer, TextPoint())
    , RangeAction(range, dir, rect, selected) {
  spaces_array_.resize(range.LineCount());
}

DecreaseIndentAction::~DecreaseIndentAction() {
}

void DecreaseIndentAction::Exec() {
  if (range_.LineCount() > 1) {
    buffer_->FreezeNotify();
  }

  effective_ = false;

  for (Coord ln = range_.line_first(); ln <= range_.line_last(); ++ln) {
    if (!buffer_->IsLineEmpty(ln, true)) {  // Skip empty lines.
      if (DecreaseIndentLine(ln)) {
        effective_ = true;
      }
    }
  }

  if (range_.LineCount() > 1) {
    buffer_->ThawNotify();
    buffer_->Notify(kLineUpdated, range_.GetLineRange());
  }
}

// Insert indented spaces back.
void DecreaseIndentAction::Undo() {
  if (range_.LineCount() > 1) {
    buffer_->FreezeNotify();
  }

  for (Coord ln = range_.line_first(); ln <= range_.line_last(); ++ln) {
    std::wstring& spaces = spaces_array_[ln - range_.line_first()];
    if (!spaces.empty()) {
      buffer_->InsertString(TextPoint(0, ln), spaces);
    }
  }

  if (range_.LineCount() > 1) {
    buffer_->ThawNotify();
    buffer_->Notify(kLineUpdated, range_.GetLineRange());
  }
}

TextPoint DecreaseIndentAction::CaretPointAfterExec() const {
  if (!range_.GetLineRange().Has(caret_point_.y)) {
    // Caret is not in the select range.
    // Normally impossible. But just check it.
    return caret_point_;
  }

  Coord ln_off = caret_point_.y - range_.line_first();
  const std::wstring& spaces = spaces_array_[ln_off];

  if (spaces.empty()) {
    // The indent of caret line was not decreased.
    return caret_point_;
  } else {
    return TextPoint(caret_point_.x - CoordCast(spaces.size()), caret_point_.y);
  }
}

TextRange DecreaseIndentAction::SelectionAfterExec() const {
  if (range_.IsEmpty()) {
    return range_;
  }

  TextPoint point_begin(range_.point_begin());
  if (!spaces_array_.front().empty()) {
    point_begin.x -= CoordCast(spaces_array_.front().size());
  }

  TextPoint point_end(range_.point_end());
  if (!spaces_array_.back().empty()) {
    point_end.x -= CoordCast(spaces_array_.back().size());
  }

  return TextRange(point_begin, point_end);
}

bool DecreaseIndentAction::DecreaseIndentLine(Coord ln) {
  TextLine* line = buffer_->Line(ln);

  Coord prefix_spaces = line->FirstNonSpaceChar();
  if (prefix_spaces == 0) {
    return false;  // No indent to decrease.
  }

  const Options& options = buffer_->ft_plugin()->options();

  // Delete from the front of line.
  Coord count = 0;
  if (line->Char(0) == kTabChar) {
    count = 1;
  } else {
    // TODO: What if the second char is tab?
    count = std::min(prefix_spaces, options.tab_stop);
  }

  Coord ln_off = ln - range_.line_first();
  buffer_->DeleteString(TextPoint(0, ln), count, &spaces_array_[ln_off]);

  return true;
}

////////////////////////////////////////////////////////////////////////////////

// Compose indent string from indent value.
std::wstring ComposeIndentString(Coord indent, bool expand_tab, int tab_stop) {
  if (expand_tab) {
    return std::wstring(indent, kSpaceChar);
  }

  std::wstring indent_str;

  Coord tabs = indent / tab_stop;
  if (tabs > 0) {
    indent_str.append(tabs, kTabChar);
  }

  Coord spaces = indent % tab_stop;
  if (spaces > 0) {
    indent_str.append(spaces, kSpaceChar);
  }

  return indent_str;
}

AutoIndentLineAction::AutoIndentLineAction(TextBuffer* buffer, Coord ln)
    : Action(buffer, TextPoint())
    , ln_(ln) {
}

AutoIndentLineAction::~AutoIndentLineAction() {
}

void AutoIndentLineAction::Exec() {
  effective_ = false;  // Reset

  if (buffer_->IsLineEmpty(ln_, true)) {
    return;
  }

  Coord indent = buffer_->GetIndent(ln_);
  Coord expected_indent = buffer_->GetExpectedIndent(ln_);

  if (indent == expected_indent) {  // TODO: Retab
    return;
  }

  // There might be multiple change notifications.
  buffer_->FreezeNotify();

  // Save old indent string.
  old_indent_str_ = buffer_->GetIndentStr(ln_);

  // Delete old indent.
  if (!old_indent_str_.empty()) {
    buffer_->DeleteString(TextPoint(0, ln_), CoordCast(old_indent_str_.size()));
  }

  // Insert new indent.
  if (expected_indent > 0) {
    const Options& options = buffer_->ft_plugin()->options();
    std::wstring indent_str = ComposeIndentString(expected_indent,
                                                  options.expand_tab,
                                                  options.tab_stop);
    buffer_->InsertString(TextPoint(0, ln_), indent_str);

    // Save new indent string.
    new_indent_str_ = indent_str;
  }

  effective_ = true;

  buffer_->ThawNotify();
  buffer_->Notify(kLineUpdated, LineRange(ln_));
}

void AutoIndentLineAction::Undo() {
  if (new_indent_str_ == old_indent_str_) {
    return;
  }

  buffer_->FreezeNotify();

  if (!new_indent_str_.empty()) {
    buffer_->DeleteString(TextPoint(0, ln_),
                          CoordCast(new_indent_str_.size()));
  }

  if (!old_indent_str_.empty()) {
    buffer_->InsertString(TextPoint(0, ln_), old_indent_str_);
  }

  buffer_->ThawNotify();
  buffer_->Notify(kLineUpdated, LineRange(ln_));
}

TextPoint AutoIndentLineAction::CaretPointAfterExec() const {
  if (ln_ != caret_point_.y) {
    // The caret point is not in this line.
    return caret_point_;
  }

  // TODO
  if (buffer_->IsLineEmpty(ln_, true)) {
    const Options& options = buffer_->ft_plugin()->options();
    return caret_point_ + TextPoint(options.shift_width, 0);
  }

  Coord delta = CoordCast(new_indent_str_.size()) -
                CoordCast(old_indent_str_.size());

  if (delta == 0) {
    return caret_point_;
  }

  return TextPoint(caret_point_.x + delta, caret_point_.y);
}

////////////////////////////////////////////////////////////////////////////////

AutoIndentAction::AutoIndentAction(TextBuffer* buffer,
                                   const TextRange& range,
                                   TextDir dir,
                                   bool rect,
                                   bool selected)
    : Action(buffer, TextPoint())
    , RangeAction(range, dir, rect, selected) {
  Coord line_count = range.LineCount();
  old_indent_strs_.resize(line_count);
  new_indent_strs_.resize(line_count);
}

AutoIndentAction::~AutoIndentAction() {
}

void AutoIndentAction::Exec() {
  // There might be multiple change notifications even when the range has
  // only one line.
  buffer_->FreezeNotify();

  effective_ = false;  // Reset

  for (Coord ln = range_.line_first(); ln <= range_.line_last(); ++ln) {
    if (!buffer_->IsLineEmpty(ln, true)) {  // Skip empty lines.
      if (AutoIndentLine(ln)) {
        effective_ = true;
      }
    }
  }

  buffer_->ThawNotify();
  buffer_->Notify(kLineUpdated, range_.GetLineRange());
}

void AutoIndentAction::Undo() {
  buffer_->FreezeNotify();

  for (Coord ln = range_.line_first(); ln <= range_.line_last(); ++ln) {
    int i = ln - range_.line_first();
    if (new_indent_strs_[i] != old_indent_strs_[i]) {
      if (!new_indent_strs_[i].empty()) {
        buffer_->DeleteString(TextPoint(0, ln),
                              CoordCast(new_indent_strs_[i].size()));
      }
      if (!old_indent_strs_[i].empty()) {
        buffer_->InsertString(TextPoint(0, ln), old_indent_strs_[i]);
      }
    }
  }

  buffer_->ThawNotify();
  buffer_->Notify(kLineUpdated, range_.GetLineRange());
}

TextPoint AutoIndentAction::CaretPointAfterExec() const {
  if (!range_.GetLineRange().Has(caret_point_.y)) {
    // Caret is not in the select range.
    // Normally impossible. But just check it.
    return caret_point_;
  }

  Coord ln_off = caret_point_.y - range_.line_first();
  Coord diff = CoordCast(new_indent_strs_[ln_off].size()) -
               CoordCast(old_indent_strs_[ln_off].size());

  if (diff != 0) {
    return TextPoint(caret_point_.x + diff, caret_point_.y);
  } else {
    return caret_point_;
  }
}

TextRange AutoIndentAction::SelectionAfterExec() const {
  if (range_.IsEmpty()) {
    return range_;
  }

  Coord new_indent = CoordCast(new_indent_strs_[0].size());
  Coord old_indent = CoordCast(old_indent_strs_[0].size());

  TextPoint point_begin(range_.point_begin());

  if (new_indent > old_indent) {
    point_begin.x += new_indent - old_indent;
  } else if (new_indent < old_indent) {
    point_begin.x -= old_indent - new_indent;
    if (point_begin.x < 0) {
      point_begin.x = 0;
    }
  }

  new_indent = CoordCast(new_indent_strs_.back().size());
  old_indent = CoordCast(old_indent_strs_.back().size());

  TextPoint point_end(range_.point_end());

  if (new_indent > old_indent) {
    point_end.x += new_indent - old_indent;
  } else if (new_indent < old_indent) {
    point_end.x -= old_indent - new_indent;
    if (point_end.x < 0) {
      point_end.x = 0;
    }
  }

  return TextRange(point_begin, point_end);
}

bool AutoIndentAction::AutoIndentLine(Coord ln) {
  const Coord indent = buffer_->GetIndent(ln);
  const Coord expected_indent = buffer_->GetExpectedIndent(ln);

  if (indent == expected_indent) {  // TODO: Retab
    return false;
  }

  const Coord ln_off = ln - range_.line_first();

  // Save old indent string.
  old_indent_strs_[ln_off] = buffer_->GetIndentStr(ln);

  // Delete old indent.
  if (!old_indent_strs_[ln_off].empty()) {
    buffer_->DeleteString(TextPoint(0, ln),
                          CoordCast(old_indent_strs_[ln_off].size()));
  }

  // Insert new indent.
  if (expected_indent > 0) {
    const Options& options = buffer_->ft_plugin()->options();
    std::wstring indent_str = ComposeIndentString(expected_indent,
                                                  options.expand_tab,
                                                  options.tab_stop);
    buffer_->InsertString(TextPoint(0, ln), indent_str);
  }

  // Save new indent string.
  new_indent_strs_[ln_off] = buffer_->GetIndentStr(ln);

  return true;
}

////////////////////////////////////////////////////////////////////////////////

SetFileFormatAction::SetFileFormatAction(TextBuffer* buffer,
                                         const TextPoint& point,
                                         FileFormat file_format)
    : Action(buffer, point)
    , file_format_(file_format)
    , old_file_format_(FF_NONE) {
}

SetFileFormatAction::~SetFileFormatAction() {
}

void SetFileFormatAction::Exec() {
  if (file_format_ == FF_NONE || file_format_ == buffer_->file_format()) {
    effective_ = false;
    return;
  }

  old_file_format_ = buffer_->file_format();
  buffer_->set_file_format(file_format_);
  buffer_->Notify(kFileFormatChange);
}

void SetFileFormatAction::Undo() {
  if (effective_) {
    buffer_->set_file_format(old_file_format_);
    buffer_->Notify(kFileFormatChange);
  }
}

}  // namespace editor
}  // namespace jil
