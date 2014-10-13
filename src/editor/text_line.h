#ifndef JIL_EDITOR_TEXT_LINE_H_
#define JIL_EDITOR_TEXT_LINE_H_
#pragma once

#include <string>
#include <list>
#include "editor/compile_config.h"
#include "editor/text_point.h"
#include "editor/text_range.h"
#include "editor/lex.h"

namespace jil {
namespace editor {

struct QuoteInfo {
  Quote* quote;
  size_t off;
  size_t len;
  Quote::Part part;
};

class TextLine {
 public:
  TextLine(size_t id, const wchar_t* data, size_t len);

  // Construct with a '\0' terminated C string.
  TextLine(size_t id, const wchar_t* data);

  TextLine(size_t id, const std::wstring& data);

  ~TextLine();

  size_t id() const { return id_; }
  void set_id(size_t id) { id_ = id; }

  const std::wstring& data() const {
    return data_;
  }

  Coord Length() const {
    return CoordCast(data_.length());
  }

  wchar_t Char(Coord off) const;

  std::wstring Sub(Coord off, Coord count) const;

  std::wstring Sub(const CharRange& char_range) const {
    return Sub(char_range.begin(), char_range.CharCount());
  }

  // Return line length on failure.
  Coord FirstNonSpaceChar(Coord off = 0) const;

  // Return kInvalidCoord on failure.
  Coord LastNonSpaceChar(Coord off = kInvalidCoord) const;

  // Return the indent as spaces.
  // Tab stop is used to expand the tabs, if any.
  Coord GetIndent(int tab_stop) const;

  // Return original indent string.
  std::wstring GetIndentStr() const;

  // Return true if the line is empty.
  // \param ignore_spaces A line with spaces only is also empty.
  bool IsEmpty(bool ignore_spaces = true) const;

  bool EndWith(wchar_t c, bool ignore_spaces = true) const;

  bool StartWith(const std::wstring& str, bool ignore_spaces = true) const;

  // Line length with tabs expanded.
  Coord TabbedLength(int tab_stop) const;

  //----------------------------------------------------------------------------

  void InsertChar(Coord off, wchar_t c);
  void DeleteChar(Coord off, wchar_t* c = NULL);

  void InsertString(Coord off, const std::wstring& str);
  void DeleteString(Coord off, Coord count, std::wstring* str = NULL);

  void Append(const std::wstring& str);

  void Clear(std::wstring* line_data = NULL);

  // Split at the given offset and return the new line.
  TextLine* Split(Coord off, size_t line_id);

  //----------------------------------------------------------------------------
  // Lex

  void AddLexElement(const LexElement& lex_element) {
    AddLexElement(lex_element.off, lex_element.len, lex_element.lex);
  }

  void AddLexElement(size_t off, size_t len, Lex lex);

  void ClearLexElements();

  const std::list<LexElement*>& lex_elements() const {
    return lex_elements_;
  }

  std::list<const LexElement*> lex_elements(const CharRange& char_range) const;

  void AddQuoteInfo(Quote* quote, size_t off, size_t len, Quote::Part part);

  void ClearQuoteInfos() {
    quote_infos_.clear();
  }

  Quote* UnendedQuote(bool multi_line) const;

  // Return true if this line ends the given quote.
  bool EndsQuote(Quote* quote) const;

  const std::list<QuoteInfo>& quote_infos() const {
    return quote_infos_;
  }

 private:
  size_t id_;
  std::wstring data_;

  std::list<LexElement*> lex_elements_;

  std::list<QuoteInfo> quote_infos_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_LINE_H_
