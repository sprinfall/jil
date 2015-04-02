#include "editor/text_range.h"
#include <algorithm>  // std::min/max

namespace jil {
namespace editor {

CharRange::CharRange(Coord begin, Coord end)
    : begin_(begin), end_(end) {
}

CharRange CharRange::Union(const CharRange& rhs) const {
  CharRange result;
  result.begin_ = std::min(begin_, rhs.begin_);
  if (end_ == kInvalidCoord) {
    result.end_ = kInvalidCoord;
  } else {
    if (rhs.end_ == kInvalidCoord) {
      result.end_ = kInvalidCoord;
    } else {
      result.end_ = std::max(end_, rhs.end_);
    }
  }
  return result;
}

CharRange CharRange::Intersect(const CharRange& rhs) const {
  CharRange result;
  result.begin_ = std::max(begin_, rhs.begin_);
  if (end_ == kInvalidCoord) {
    result.end_ = rhs.end_;
  } else {
    if (rhs.end_ == kInvalidCoord) {
      result.end_ = end_;
    } else {
      result.end_ = std::min(end_, rhs.end_);
    }
  }
  if (result.end_ != kInvalidCoord && result.end_ < result.begin_) {
    result.end_ = result.begin_;
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////

void LineRange::Set(Coord first, Coord last) {
  first_ = first;
  last_ = last;

  if (last_ < first_) {
    last_ = first_;
  }
}

bool LineRange::Contains(const LineRange& rhs) const {
  if (IsEmpty() || rhs.IsEmpty()) {
    return false;
  }

  return (first_ <= rhs.first_ && last_ >= rhs.last_);
}

LineRange LineRange::Union(const LineRange& rhs) const {
  return LineRange(std::min(first_, rhs.first_), std::max(last_, rhs.last_));
}

LineRange LineRange::Intersect(const LineRange& rhs) const {
  Coord first = std::max(first_, rhs.first_);
  Coord last = std::min(last_, rhs.last_);
  if (first > last) {
    return LineRange();
  } else {
    return LineRange(first, last);
  }
}

void LineRange::Substract(const LineRange& rhs,
                          LineRange* sub1,
                          LineRange* sub2) const {
  assert(Contains(rhs));

  if (first_ < rhs.first_) {
    sub1->Set(first_, rhs.first_ - 1);
  }

  if (last_ > rhs.last_) {
    sub2->Set(rhs.last_ + 1, last_);
  }
}

wxString LineRange::ToString() const {
  return wxString::Format(wxT("[%d, %d]"), first_, last_);
}

////////////////////////////////////////////////////////////////////////////////

CharRange TextRange::GetCharRange(Coord ln) const {
  assert(HasLine(ln));

  if (point_begin_.y == point_end_.y) {  // Single line.
    return CharRange(point_begin_.x, point_end_.x);
  } else if (ln == point_begin_.y) {
    return CharRange(point_begin_.x, kInvalidCoord);
  } else if (ln == point_end_.y) {
    return CharRange(0, point_end_.x);
  } else {  // Middle line.
    return CharRange(0, kInvalidCoord);
  }
}

CharRange TextRange::GetCharRange() const {
  Coord x1 = point_begin_.x;
  Coord x2 = point_end_.x;
  if (x1 > x2) {
    std::swap(x1, x2);
  }
  return CharRange(x1, x2);
}

wxString TextRange::ToString() const {
  wxString str = wxT("[");
  str += point_begin_.ToString();
  str += wxT("-");
  str += point_end_.ToString();
  str += wxT(")");
  return str;
}

}  // namespace editor
}  // namespace jil
