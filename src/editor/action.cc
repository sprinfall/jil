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
    , text_unit_(text_unit)
    , seek_type_(seek_type)
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
  const TextOptions& options = buffer_->text_options();

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

  const TextOptions& options = buffer_->text_options();
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

  Coord prefix_spaces = line->FindNonSpace();
  if (prefix_spaces == 0) {
    return false;  // No indent to decrease.
  }

  const TextOptions& options = buffer_->text_options();

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
    const TextOptions& options = buffer_->text_options();
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
    buffer_->DeleteString(TextPoint(0, ln_), CoordCast(new_indent_str_.size()));
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
    return caret_point_ + TextPoint(buffer_->tab_stop(), 0);
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
    const TextOptions& options = buffer_->text_options();
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

CommentAction::CommentAction(TextBuffer* buffer,
                             const TextRange& range,
                             TextDir dir,
                             bool rect,
                             bool selected)
    : Action(buffer, TextPoint())
    , RangeAction(range, dir, rect, selected)
    , point_begin_delta_(0, 0)
    , point_end_delta_(0, 0) {
}

CommentAction::~CommentAction() {
}

void CommentAction::Exec() {
  change_infos_.clear();

  effective_ = true;

  if (range_.IsEmpty()) {
    CommentLines(range_.GetLineRange());
    return;
  }

  // Copy the range to avoid changing it.
  TextRange range = range_;

  if (range.point_end().x == 0) {
    // If the range ends at the beginning of a line, don't take that line
    // into account.
    Coord ln = range.line_last() - 1;
    TextPoint point_end(buffer_->LineLength(ln), ln);
    range.Set(range.point_begin(), point_end);

    // The range might be empty after this change.
    if (range.IsEmpty()) {
      CommentLines(range.GetLineRange());
      return;
    }
  }

  bool by_line = ByLine(range);

  buffer_->FreezeNotify();

  if (by_line) {
    CommentLines(range.GetLineRange());
  } else {
    CommentBlock(range);
  }

  buffer_->ThawNotify();
  buffer_->Notify(kLineUpdated, range.GetLineRange());
}

void CommentAction::Undo() {
  buffer_->FreezeNotify();

  // Use reverse iterator in case there are changes at the same line.
  std::list<ChangeInfo>::reverse_iterator it = change_infos_.rbegin();
  for (; it != change_infos_.rend(); ++it) {
    buffer_->DeleteString(it->first, it->second);
  }

  buffer_->ThawNotify();
  buffer_->Notify(kLineUpdated, range_.GetLineRange());
}

TextPoint CommentAction::CaretPointAfterExec() const {
  return caret_point_ + delta_point_;
}

TextRange CommentAction::SelectionAfterExec() const {
  if (range_.IsEmpty()) {
    return range_;
  }

  TextPoint point_begin(range_.point_begin() + point_begin_delta_);
  TextPoint point_end(range_.point_end() + point_end_delta_);
  return TextRange(point_begin, point_end);
}

bool CommentAction::IsComment(const TextPoint& point) const {
  return (buffer_->GetLex(point).major() == kLexComment);
}

void CommentAction::CommentLines(const LineRange& line_range) {
  const LexComment& sline_comment = GetSlineComment();

  if (sline_comment.IsEmpty()) {
    // No single line comment available.
    TextPoint point_begin(0, line_range.first());
    Coord y = line_range.last();
    TextPoint point_end(buffer_->LineLength(y), y);
    CommentBlock(TextRange(point_begin, point_end));
  } else {
    TextPoint p(0, 1);
    if (buffer_->text_options().comment_respect_indent) {
      p.x = GetMinIndent(line_range);
    }

    std::wstring comment_start = sline_comment.start;
    if (buffer_->text_options().comment_add_space) {
      comment_start.append(1, kSpaceChar);
    }

    for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
      p.y = ln;
      Insert(p, comment_start);
    }
  }
}

void CommentAction::CommentBlock(const TextRange& range) {
  const LexComment& block_comment = GetBlockComment();

  std::wstring comment_start = block_comment.start;
  std::wstring comment_end = block_comment.end;

  if (buffer_->text_options().comment_add_space) {
    comment_start.append(1, kSpaceChar);
    comment_end.insert(comment_end.begin(), 1, kSpaceChar);
  }

  Insert(range.point_begin(), comment_start);

  TextPoint point_end = range.point_end();
  if (range.LineCount() == 1) {
    point_end.x += CoordCast(comment_start.size());
  }
  Insert(point_end, comment_end);
}

bool CommentAction::ByLine(const TextRange& range) const {
  if (GetBlockComment().IsEmpty()) {
    // No block comment available.
    return true;
  }

  if (range.point_begin().x == 0 &&
      range.point_end().x == buffer_->LineLength(range.point_end().y)) {
    return true;
  }

  if (IsComment(range.point_begin()) || IsComment(range.point_end())) {
    return true;
  }

  return false;
}

void CommentAction::Insert(const TextPoint& point, const std::wstring& str) {
  buffer_->InsertString(point, str);

  Coord str_size = CoordCast(str.size());

  change_infos_.push_back(std::make_pair(point, str_size));

  if (point.y == caret_point_.y) {
    if (point.x <= caret_point_.x) {
      delta_point_.Set(str_size, 0);
    }
  }

  if (point.y == range_.point_begin().y) {
    if (point.x <= range_.point_begin().x) {
      point_begin_delta_.Set(str_size, 0);
    }
  }

  if (point.y == range_.point_end().y) {
    if (point.x < range_.point_end().x) {  // NOTE: < instead of <=
      point_end_delta_.Set(str_size, 0);
    }
  }
}

const LexComment& CommentAction::GetSlineComment() const {
  return buffer_->ft_plugin()->sline_comment();
}

const LexComment& CommentAction::GetBlockComment() const {
  return buffer_->ft_plugin()->block_comment();
}

Coord CommentAction::GetMinIndent(const LineRange& line_range) const {
  Coord min_indent = kInvCoord;
  Coord indent = 0;

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    if (!buffer_->IsLineEmpty(ln, true)) {
      indent = buffer_->GetIndentStrLength(ln);

      if (min_indent == kInvCoord) {
        min_indent = indent;
      } else if (min_indent > indent) {
        min_indent = indent;
      }
    }
  }

  return min_indent;
}

////////////////////////////////////////////////////////////////////////////////

UncommentAction::UncommentAction(TextBuffer* buffer,
                                 const TextRange& range,
                                 TextDir dir,
                                 bool rect,
                                 bool selected)
    : Action(buffer, TextPoint())
    , RangeAction(range, dir, rect, selected)
    , refresh_line_range_(kInvCoord, 0)
    , point_begin_delta_(0, 0)
    , point_end_delta_(0, 0) {
}

UncommentAction::~UncommentAction() {
}

void UncommentAction::Exec() {
  refresh_line_range_.Set(kInvCoord, 0);
  change_infos_.clear();

  effective_ = false;  // Reset

  // Copy the range to avoid changing it.
  TextRange range = range_;

  // If the range ends at the beginning of a line, don't take that line
  // into account.
  if (!range_.IsEmpty() && range.point_end().x == 0) {
    Coord ln = range.line_last() - 1;
    TextPoint point_end(buffer_->LineLength(ln), ln);
    range.Set(range.point_begin(), point_end);
  }

  // Trim the spaces of the range.
  if (!range.IsEmpty()) {
    range = TrimRange(range);
  }

  // Try to uncomment by block.
  if (IsComment(range.point_begin())) {
    QuoteInfo quote_info;
    if (buffer_->GetQuoteInfo(range.point_begin(), &quote_info)) {
      TextPoint quote_end_point = quote_info.end_point;
      quote_end_point.x += quote_info.end_len;

      if (range.point_begin() >= quote_info.start_point &&
          range.point_end() <= quote_end_point) {
        buffer_->FreezeNotify();

        // Delete quote end firstly in case they are at the same line.
        if (quote_info.end_len != 0) {
          Delete(quote_info.end_point, quote_info.end_len);
        }

        // Delete quote start.
        Delete(quote_info.start_point, quote_info.start_len);

        buffer_->ThawNotify();
        if (!refresh_line_range_.IsEmpty()) {
          buffer_->Notify(kLineUpdated, refresh_line_range_);
        }

        return;
      }
    }
  }

  // Uncomment by line.
  UncommentLines(range.GetLineRange());
}

void UncommentAction::Undo() {
  buffer_->FreezeNotify();

  // Use reverse iterator in case there are changes at the same line.
  std::list<ChangeInfo>::reverse_iterator it = change_infos_.rbegin();
  for (; it != change_infos_.rend(); ++it) {
    buffer_->InsertString(it->first, it->second);
  }

  buffer_->ThawNotify();
  if (!refresh_line_range_.IsEmpty()) {
    buffer_->Notify(kLineUpdated, refresh_line_range_);
  }
}

TextPoint UncommentAction::CaretPointAfterExec() const {
  return caret_point_ + delta_point_;
}

TextRange UncommentAction::SelectionAfterExec() const {
  if (range_.IsEmpty()) {
    return range_;
  }

  TextPoint point_begin(range_.point_begin() + point_begin_delta_);
  TextPoint point_end(range_.point_end() + point_end_delta_);
  return TextRange(point_begin, point_end);
}

bool UncommentAction::IsComment(const TextPoint& point) const {
  return (buffer_->GetLex(point).major() == kLexComment);
}

TextRange UncommentAction::TrimRange(const TextRange& range) const {
  TextPoint point_begin = range.point_begin();
  TextPoint point_end = range.point_end();

  TextLine* begin_line = buffer_->Line(point_begin.y);
  TextLine* end_line = buffer_->Line(point_end.y);

  point_end.x = end_line->FindLastNonSpace(point_end.x);

  if (point_begin.y == point_end.y) {
    // Avoid to trim to an invalid text range.
    if (point_begin.x < point_end.x) {
      point_begin.x = begin_line->FindNonSpace(point_begin.x);
    }
    if (point_begin.x > point_end.x) {
      point_begin.x = point_end.x;
    }
  } else {
    point_begin.x = begin_line->FindNonSpace(point_begin.x);
  }

  return TextRange(point_begin, point_end);
}

void UncommentAction::UncommentLines(const LineRange& line_range) {
  buffer_->FreezeNotify();

  for (Coord ln = line_range.first(); ln <= line_range.last(); ++ln) {
    TextLine* line = buffer_->Line(ln);

    Coord x = line->FindNonSpace();
    if (x == line->Length()) {
      continue;
    }

    const QuoteElem* quote_start = NULL;
    const QuoteElem* quote_end = NULL;
    if (!line->GetQuoteElem(x, &quote_start, &quote_end)) {
      continue;
    }

    if (quote_start == NULL || quote_end == NULL) {
      // The quote starts or ends in another line.
      continue;
    }

    if (quote_start->quote->lex().major() != kLexComment) {
      continue;
    }

    if (quote_start->off != x) {
      // Non-space char before the quote start.
      continue;
    }

    Coord quote_end_off = quote_end->off + quote_end->len;
    if (line->FindNonSpace(quote_end_off) != line->Length()) {
      // Non-space char after the quote end.
      continue;
    }

    // Delete quote end firstly in case they are at the same line.
    if (quote_end->len != 0) {
      Delete(TextPoint(quote_end->off, ln), quote_end->len);
    }

    // Delete quote start.
    Delete(TextPoint(quote_start->off, ln), quote_start->len);
  }

  buffer_->ThawNotify();
  if (!refresh_line_range_.IsEmpty()) {
    buffer_->Notify(kLineUpdated, refresh_line_range_);
  }
}

void UncommentAction::Delete(const TextPoint& point, Coord count) {
  if (!effective_) {
    effective_ = true;
  }

  // Update the refresh line range.
  if (refresh_line_range_.first() == kInvCoord ||
      refresh_line_range_.first() > point.y) {
    refresh_line_range_.set_first(point.y);
  }
  if (refresh_line_range_.last() < point.y) {
    refresh_line_range_.set_last(point.y);
  }

  std::wstring str;
  buffer_->DeleteString(point, count, &str);

  change_infos_.push_back(std::make_pair(point, str));

  if (caret_point_.y == point.y) {
    if (caret_point_.x > point.x) {
      if (caret_point_.x >= point.x + count) {
        delta_point_.Set(-count, 0);
      } else {
        delta_point_.Set(point.x - caret_point_.x, 0);
      }
    }
  }

  if (range_.point_begin().y == point.y) {
    if (range_.point_begin().x >= point.x) {
      if (range_.point_begin().x >= point.x + count) {
        point_begin_delta_.Set(-count, 0);
      } else {
        point_begin_delta_.Set(point.x - range_.point_begin().x, 0);
      }
    }
  }

  if (range_.point_end().y == point.y) {
    if (range_.point_end().x >= point.x) {
      if (range_.point_end().x >= point.x + count) {
        point_end_delta_.Set(-count, 0);
      } else {
        point_end_delta_.Set(point.x - range_.point_end().x, 0);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

RetabAction::RetabAction(TextBuffer* buffer)
    : Action(buffer, TextPoint())
    , refresh_line_range_(kInvCoord, 0) {
}

RetabAction::~RetabAction() {
}

void RetabAction::Exec() {
  refresh_line_range_.Set(kInvCoord, 0);
  change_infos_.clear();

  effective_ = false;  // Reset

  buffer_->FreezeNotify();

  int tab_stop = buffer_->tab_stop();
  Coord line_count = buffer_->LineCount();

  if (buffer_->expand_tab()) {
    for (Coord ln = 1; ln <= line_count; ++ln) {
      ToSpaces(ln, tab_stop);
    }
  } else {
    for (Coord ln = 1; ln <= line_count; ++ln) {
      ToTabs(ln, tab_stop);
    }
  }

  buffer_->ThawNotify();
  if (!refresh_line_range_.IsEmpty()) {
    buffer_->Notify(kLineUpdated, refresh_line_range_);
  }
}

void RetabAction::Undo() {
  buffer_->FreezeNotify();

  TextLine* line = NULL;
  for (ChangeInfo& ci : change_infos_) {
    line = buffer_->Line(ci.first);
    line->DeleteString(0, line->GetIndentStrLength());
    line->InsertString(0, ci.second);
  }

  buffer_->ThawNotify();
  if (!refresh_line_range_.IsEmpty()) {
    buffer_->Notify(kLineUpdated, refresh_line_range_);
  }
}

TextPoint RetabAction::CaretPointAfterExec() const {
  TextPoint p = caret_point_;
  TextLine* line = buffer_->Line(p.y);
  if (p.x > line->Length()) {
    p.x = line->Length();
  }
  return p;
}

void RetabAction::ToSpaces(Coord ln, int tab_stop) {
  TextLine* line = buffer_->Line(ln);

  IndentProp indent = line->GetIndentProp();
  if (indent.length == 0 || indent.type == kSpaceIndent) {
    return;
  }

  Coord expanded_length = 0;

  for (Coord x = 0; x < indent.length; ++x) {
    if (line->Char(x) == kTabChar) {
      expanded_length += tab_stop - (expanded_length % tab_stop);
    } else {
      ++expanded_length;
    }
  }

  std::wstring str;
  line->DeleteString(0, indent.length, &str);
  line->InsertString(0, std::wstring(expanded_length, L' '));

  change_infos_.push_back(std::make_pair(ln, str));

  if (!effective_) {
    effective_ = true;
  }

  UpdateRefreshLineRange(ln);
}

void RetabAction::ToTabs(Coord ln, int tab_stop) {
  TextLine* line = buffer_->Line(ln);

  IndentProp indent = line->GetIndentProp();
  if (indent.length == 0 || indent.type == kTabIndent) {
    return;
  }

  int indent_size = line->GetIndent(tab_stop);

  int tabs = indent_size / tab_stop;
  int spaces = indent_size % tab_stop;

  std::wstring new_indent_str;
  if (tabs > 0) {
    new_indent_str.append(tabs, kTabChar);
  }
  if (spaces > 0) {
    new_indent_str.append(spaces, kSpaceChar);
  }

  std::wstring old_indent_str;
  line->DeleteString(0, indent.length, &old_indent_str);
  line->InsertString(0, new_indent_str);

  change_infos_.push_back(std::make_pair(ln, old_indent_str));

  if (!effective_) {
    effective_ = true;
  }

  UpdateRefreshLineRange(ln);
}

void RetabAction::UpdateRefreshLineRange(Coord ln) {
  if (refresh_line_range_.first() == kInvCoord ||
      refresh_line_range_.first() > ln) {
    refresh_line_range_.set_first(ln);
  }

  if (refresh_line_range_.last() < ln) {
    refresh_line_range_.set_last(ln);
  }
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
