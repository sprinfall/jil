#ifndef JIL_EDITOR_WRAP_H_
#define JIL_EDITOR_WRAP_H_
#pragma once

#include <string>
#include <vector>
#include <deque>
#include "editor/text_range.h"

namespace jil {
namespace editor {

class TextExtent;
class TextBuffer;

// About "wrap delta":
// = 0: Wrap is not changed
// < 0: Wrap is decreased
// > 0: Wrap is increased

// Wrap offsets.
// Example:
// If a line "ABCDEFGHIJ" is wrapped as:
// ABC
// DEF
// GHI
// J
// The wrap offsets is: [3, 6, 9].
typedef std::vector<Coord> WrapOffsets;

WrapOffsets WrapLineByChar(const std::wstring& line,
                           TextExtent* text_extent,
                           Coord max_width);

// Wrap information for a text line.
class WrapInfo {
public:
  const WrapOffsets& offsets() const {
    return offsets_;
  }

  Coord WrapCount() const {
    return CoordCast(offsets_.size());
  }

  // Wrap a line.
  // @delta is the wrap count increased (> 0) or decreased (< 0).
  // Return true if the wrap offsets change.
  bool Wrap(const std::wstring& line,
            TextExtent* text_extent,
            Coord max_width,
            int* delta);

  // Return the wrap count before unwrap.
  Coord Unwrap();

  // Get ranges of sub lines.
  // Example:
  // Offsets: [3, 6, 9] -> Sub ranges: [<0, 3>, <3, 6>, <6, 9>, <9, npos/-1>]
  std::vector<CharRange> GetSubRanges() const;

  // Example:
  // Given line "ABCDEFGHIJ" wrapped as:
  //       sub ln
  // --------------
  // ABC      1
  // DEF      2
  // GHI      3
  // J        4
  // GetSubLineNr(4, ...) -> 2; sub_off: 3.
  // GetSubLineNr(7, ...) -> 3; sub_off: 6.
  // Return the sub ln.
  Coord GetSubLineNr(Coord x, Coord* sub_off = NULL) const;

  size_t GetOffset(Coord sub_ln) const {
    assert(sub_ln >= 1 && sub_ln <= WrapCount() + 1);
    return sub_ln == 1 ? 0 : offsets_[sub_ln - 1];
  }

private:
  WrapOffsets offsets_;
};

class WrapHelper {
public:
  WrapHelper(TextBuffer* buffer, TextExtent* text_extent);
  ~WrapHelper();

  int client_width() const {
    return client_width_;
  }

  void set_client_width(int client_width) {
    client_width_ = client_width;
  }

  const WrapInfo& GetWrapInfo(Coord ln) const {
    assert(ln <= CoordCast(wrap_infos_.size()));
    return wrap_infos_[ln - 1];
  }

  Coord WrapCount(Coord ln) const {
    return GetWrapInfo(ln).WrapCount();
  }

  // Return the wrap delta.
  int AddLineWrap(Coord ln);

  void RemoveLineWrap(Coord ln);

  // Return the wrap delta.
  int UpdateLineWrap(Coord ln);

  // Example:
  // Given client width as 3 * char width.
  // Before wrap:
  //  2 DEFGHIJ
  // After wrap:
  //  2 DEF
  //    GHI
  //    J
  // WrappedLineCount(2) returns 3.
  Coord WrappedLineCount(Coord ln) const {
    return WrapCount(ln) + 1;
  }

  // Get sub ranges of a given line.
  // Example:
  //   Wrap offsets: [3, 6, 9] ->
  //   Sub ranges: [<0, 3>, <3, 6>, <6, 9>, <9, npos/-1>]
  std::vector<CharRange> SubRanges(Coord ln) const {
    return GetWrapInfo(ln).GetSubRanges();
  }

  // Example:
  // Given line 2 "ABCDEFGHIJ" wrapped as:
  //          sub ln
  //    --------------
  // 2  ABC      1
  //    DEF      2
  //    GHI      3
  //    J        4
  // SubLineNr(2, 4, ...) -> 2; sub_off: 3.
  // SubLineNr(2, 7, ...) -> 3; sub_off: 6.
  Coord SubLineNr(Coord ln, Coord x, Coord* sub_off = NULL) const {
    return GetWrapInfo(ln).GetSubLineNr(x, sub_off);
  }

  // Given client width as 3 * char width.
  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC
  //  2 DEF
  //    GHI
  //    J
  //  5 KLM
  //    N
  // WrappedLineCount() returns 6.
  Coord WrappedLineCount() const {
    return wrapped_line_count_;
  }

  // Wrap all lines.
  // @wrap_delta is the wrap count increased (> 0) or decreased (< 0).
  // Return true if the wrap offsets change.
  bool Wrap(int* wrap_delta);

  // Unwrap all lines.
  // @wrap_delta is the wrap count increased (> 0) or decreased (< 0).
  // Return true if the wrap offsets change.
  bool Unwrap(int* wrap_delta);

  Coord WrapLineNr(Coord ln) const;

  // Given 3 lines wrapped as:
  //  1 ABC
  //  2 DEF
  //  | GHI  (wrap_ln: 3)
  //  | J
  //  3 KLM
  //  | N
  // UnwrapLineNr(3) returns 2 and set sub_ln to 2.
  Coord UnwrapLineNr(Coord wrapped_ln, Coord* sub_ln) const;

  // Wrap a line range.
  // Examples:
  // Given client width as 3 * char width.
  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC
  //  2 DEF
  //  | GHI
  //  | J
  //  3 KLM
  //  | N
  // We have:
  //  - WrapLineRange([1, 1]) -> [1, 1]
  //  - WrapLineRange([1, 2]) -> [1, 4]
  //  - WrapLineRange([2, 3]) -> [2, 6]
  //  - WrapLineRange([3, 3]) -> [5, 6]
  //  - WrapLineRange([4, 5]) -> [0, 0]
  LineRange WrapLineRange(const LineRange& line_range) const;

  // Unwrap a line range.
  // \param wrapped_line_range Might be partial.
  // Examples:
  // Given client width as 3 * char width.
  // Before wrap:
  //  1 ABC
  //  2 DEFGHIJ
  //  3 KLMN
  // After wrap:
  //  1 ABC
  //  2 DEF
  //  | GHI
  //  | J
  //  3 KLM
  //  | N
  // We have:
  //  - UnwrapLineRange([1, 1]) -> [1, 1]
  //  - UnwrapLineRange([1, 2]) -> [1, 2]
  //  - UnwrapLineRange([1, 3]) -> [1, 2]
  //  - UnwrapLineRange([3, 4]) -> [2, 2]
  //  - UnwrapLineRange([2, 8]) -> [2, 3]
  //  - UnwrapLineRange([10, 20]) -> [0, 0]
  LineRange UnwrapLineRange(const LineRange& wrapped_line_range) const;

private:
  TextBuffer* buffer_;
  TextExtent* text_extent_;
  int client_width_;

  typedef std::deque<WrapInfo> WrapInfos;
  WrapInfos wrap_infos_;

  // Cache the wrapped line count.
  Coord wrapped_line_count_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_WRAP_H_
