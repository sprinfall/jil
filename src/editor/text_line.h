#ifndef JIL_EDITOR_TEXT_LINE_H_
#define JIL_EDITOR_TEXT_LINE_H_
#pragma once

#include <list>
#include <string>
#include <vector>

#include "editor/compile_config.h"
#include "editor/text_point.h"
#include "editor/text_range.h"
#include "editor/lex.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

struct QuoteInfo {
  Quote* quote;
  size_t off;
  size_t len;
  QuotePart part;
};

////////////////////////////////////////////////////////////////////////////////

class TextLine {
public:
  TextLine(size_t id, const wchar_t* data, size_t len);

  // Construct with a '\0' terminated C string.
  TextLine(size_t id, const wchar_t* data);

  TextLine(size_t id, const std::wstring& data);

  ~TextLine();

  size_t id() const {
    return id_;
  }
  void set_id(size_t id) {
    id_ = id;
  }

  const std::wstring& data() const {
    return data_;
  }

  Coord Length() const {
    return CoordCast(data_.length());
  }

  // Line length with tabs expanded.
  Coord TabbedLength(int tab_stop, Coord count = kInvalidCoord) const;

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
  // \param ignore_spaces A line is empty if it has only empty spaces.
  bool IsEmpty(bool ignore_spaces) const;

  // NOTE: ignore_spaces will be ignored if c is an empty space.
  bool StartWith(wchar_t c, bool ignore_spaces, Coord* off = NULL) const;

  // NOTE: ignore_spaces will be ignored if str[0] is an empty space.
  bool StartWith(const std::wstring& str,
                 bool ignore_spaces,
                 Coord* off = NULL) const;

  // NOTE: ignore_spaces will be ignored if c is an empty space.
  bool EndWith(wchar_t c,
               bool ignore_comments,
               bool ignore_spaces,
               Coord* off = NULL) const;

  // NOTE: ignore_spaces will be ignored if str[str.size()-1] is an empty space.
  bool EndWith(const std::wstring& str,
               bool ignore_comments,
               bool ignore_spaces,
               Coord* off = NULL) const;

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

  // Return the lex elements inside the given char range.
  std::list<const LexElement*> lex_elements(const CharRange& char_range) const;

  // Return true if this line only has spaces.
  bool SpacesOnly() const;

  // Return true if this line only has comments.
  bool CommentsOnly() const;

  void AddQuoteInfo(Quote* quote, size_t off, size_t len, QuotePart part);

  void ClearQuoteInfos() {
    quote_infos_.clear();
  }

  Quote* UnendedQuote(bool multi_line) const;

  // Return true if this line ends the given quote.
  bool EndQuote(Quote* quote) const;

  const std::list<QuoteInfo>& quote_infos() const {
    return quote_infos_;
  }

private:
  size_t id_;
  std::wstring data_;

  std::list<LexElement*> lex_elements_;

  std::list<QuoteInfo> quote_infos_;
};

////////////////////////////////////////////////////////////////////////////////

class LinePred {
public:
  virtual bool Check(const TextLine* line) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class LineStartWith : public LinePred {
public:
  LineStartWith(const std::wstring& str1) {
    strs_.push_back(str1);
  }

  LineStartWith(const std::wstring& str1, const std::wstring& str2) {
    strs_.push_back(str1);
    strs_.push_back(str2);
  }

  LineStartWith(const std::wstring& str1,
                const std::wstring& str2,
                const std::wstring& str3) {
    strs_.push_back(str1);
    strs_.push_back(str2);
    strs_.push_back(str3);
  }

  virtual bool Check(const TextLine* line) const override {
    for (size_t i = 0; i < strs_.size(); ++i) {
      if (line->StartWith(strs_[i], true)) {
        return true;
      }
    }
    return false;
  }

private:
  std::vector<std::wstring> strs_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_LINE_H_
