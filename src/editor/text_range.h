#ifndef JIL_EDITOR_TEXT_RANGE_H_
#define JIL_EDITOR_TEXT_RANGE_H_
#pragma once

#include <cassert>
#include "editor/text_point.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

// The range inside a single line.
// STL style begin/end range: [begin, end).
class CharRange {
public:
  CharRange(Coord begin = 0, Coord end = kInvalidCoord);

  Coord begin() const { return begin_; }
  void set_begin(Coord begin) { begin_ = begin; }

  Coord end() const { return end_; }
  void set_end(Coord end) { end_ = end; }

  void Set(Coord begin, Coord end) {
    begin_ = begin;
    end_ = end;
  }

  Coord CharCount() const {
    return end_ == kInvalidCoord ? kInvalidCoord : (end_ - begin_);
  }

  CharRange Union(const CharRange& rhs);
  CharRange Intersect(const CharRange& rhs);

  bool IsEmpty() const {
    return end_ != kInvalidCoord && begin_ >= end_;
  }

private:
  Coord begin_;
  Coord end_;
};

inline bool operator==(const CharRange& lhs, const CharRange& rhs) {
  return lhs.begin() == rhs.begin() && lhs.end() == rhs.end();
}

inline bool operator!=(const CharRange& lhs, const CharRange& rhs) {
  return !(lhs == rhs);
}

////////////////////////////////////////////////////////////////////////////////

// Range of line.
// [first, last] with first <= last.
class LineRange {
public:
  LineRange(Coord first = 0, Coord last = 0)
      : first_(first), last_(last) {
    if (last_ < first_) {
      last_ = first_;
    }
  }

  LineRange(const LineRange& rhs)
      : first_(rhs.first_), last_(rhs.last_) {
  }

  Coord first() const { return first_; }
  void set_first(Coord first) { first_ = first; }

  Coord last() const { return last_; }
  void set_last(Coord last) { last_ = last; }

  void Set(Coord first, Coord last = 0);

  bool IsEmpty() const {
    return first_ <= 0 || last_ <= 0 || first_ > last_;
  }

  Coord LineCount() const {
    return IsEmpty() ? 0 : (last_ + 1 - first_);
  }

  bool Has(Coord ln) const {
    return ln >= first_ && ln <= last_;
  }

  bool Contains(const LineRange& rhs) const;

  LineRange Union(const LineRange& rhs) const;
  LineRange Intersect(const LineRange& rhs) const;
  void Substract(const LineRange& rhs, LineRange* sub1, LineRange* sub2) const;

  wxString ToString() const;

private:
  Coord first_;
  Coord last_;
};

inline bool operator==(const LineRange& lhs, const LineRange& rhs) {
  return lhs.first() == rhs.first() && lhs.last() == rhs.last();
}

inline bool operator!=(const LineRange& lhs, const LineRange& rhs) {
  return !(lhs == rhs);
}

////////////////////////////////////////////////////////////////////////////////

// Range of text.
// STL style range: [point_begin, point_end).
class TextRange {
public:
  // Construct an invalid text range.
  TextRange() {
  }

  // NOTE: point_begin <= point_end.
  TextRange(const TextPoint& point_begin, const TextPoint& point_end)
      : point_begin_(point_begin), point_end_(point_end) {
    assert(point_begin_ <= point_end_);
  }

  TextRange(const TextRange& rhs)
      : point_begin_(rhs.point_begin_), point_end_(rhs.point_end_) {
  }

  Coord LineCount() const {
    return point_end_.y + 1 - point_begin_.y;
  }

  bool IsEmpty() const {
    return point_begin_.y < 1 ||
           point_end_.y < 1 ||
           point_begin_ >= point_end_;
  }

  bool Contain(const TextPoint& point) const {
    return point >= point_begin_ && point < point_end_;
  }

  const TextPoint& point_begin() const {
    return point_begin_;
  }
  const TextPoint& point_end() const {
    return point_end_;
  }

  void Set(const TextPoint& point_begin, const TextPoint& point_end) {
    assert(point_begin <= point_end);
    point_begin_ = point_begin;
    point_end_ = point_end;
  }

  void Reset() {
    point_begin_.Reset();
    point_end_.Reset();
  }

  LineRange GetLineRange() const {
    return LineRange(point_begin_.y, point_end_.y);
  }

  // Get the char range of the given line.
  CharRange GetCharRange(Coord ln) const;

  // Get the char range between the begin and end points.
  CharRange GetCharRange() const;

  Coord line_first() const {
    return point_begin_.y;
  }
  Coord line_last() const {
    return point_end_.y;
  }

  // If the given line is in this text range.
  bool HasLine(Coord ln) const {
    return ln >= point_begin_.y && ln <= point_end_.y;
  }

  // If this text range has any line.
  bool HasAnyLine() const {
    return point_begin_.y > 0 &&
           point_end_.y > 0 &&
           point_begin_.y <= point_end_.y;
  }

  wxString ToString() const;

private:
  TextPoint point_begin_;
  TextPoint point_end_;
};

inline bool operator==(const TextRange& lhs, const TextRange& rhs) {
  return (lhs.point_begin() == rhs.point_begin() &&
          lhs.point_end() == rhs.point_end());
}

inline bool operator!=(const TextRange& lhs, const TextRange& rhs) {
  return !(lhs == rhs);
}

inline bool operator<(const TextRange& lhs, const TextRange& rhs) {
  return (lhs.point_begin().y < rhs.point_begin().y);
}

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_RANGE_H_
