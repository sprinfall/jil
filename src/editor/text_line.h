#ifndef JIL_EDITOR_TEXT_LINE_H_
#define JIL_EDITOR_TEXT_LINE_H_
#pragma once

#include <list>
#include <string>

#include "boost/any.hpp"

extern "C" {
#include "lua.h"
}

#include "editor/compile_config.h"
#include "editor/text_point.h"
#include "editor/text_range.h"
#include "editor/lex.h"

namespace jil {
namespace editor {

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

  const boost::any& extra_data() const {
    return extra_data_;
  }

  void set_extra_data(const boost::any& extra_data) {
    extra_data_ = extra_data;
  }

  //----------------------------------------------------------------------------

  Coord Length() const {
    return CoordCast(data_.length());
  }

  // Line length with tabs expanded.
  Coord TabbedLength(int tab_stop, Coord count = kInvCoord) const;

  wchar_t Char(Coord off) const;

  std::wstring Sub(Coord off, Coord count) const;

  std::wstring Sub(const CharRange& char_range) const {
    return Sub(char_range.begin(), char_range.CharCount());
  }

  // Return line length on failure.
  Coord FindNonSpace(Coord off = 0) const;

  // Return kInvCoord on failure.
  Coord FindLastNonSpace(Coord off = kInvCoord) const;

  Coord FindLastChar(wchar_t c,
                     bool ignore_comment,
                     Coord off = kInvCoord) const;

  // Return true if the line is empty.
  // \param ignore_space A line is empty if it has only empty spaces.
  bool IsEmpty(bool ignore_space) const;

  // NOTE: ignore_space will be ignored if c is an empty space.
  bool StartWith(wchar_t c,
                 bool ignore_comment,
                 bool ignore_space,
                 Coord* off = NULL) const;

  // NOTE: ignore_space will be ignored if str[0] is an empty space.
  bool StartWith(const std::wstring& str,
                 bool ignore_comment,
                 bool ignore_space,
                 Coord* off = NULL) const;

  // NOTE: ignore_space will be ignored if c is an empty space.
  bool EndWith(wchar_t c,
               bool ignore_comment,
               bool ignore_space,
               Coord* off = NULL) const;

  // NOTE: ignore_space will be ignored if str[str.size()-1] is an empty space.
  bool EndWith(const std::wstring& str,
               bool ignore_comment,
               bool ignore_space,
               Coord* off = NULL) const;

  bool Equal(const std::wstring& str,
             bool ignore_comment,
             bool ignore_space) const;

  // Return true if the last char is an unescaped back slash.
  bool IsEolEscaped(bool no_comment_or_string) const;

  Coord UnpairedLeftKey(wchar_t l_key,
                        wchar_t r_key,
                        Coord off = kInvCoord) const;

  // Return the indent as spaces.
  // Tab stop is used to expand the tabs, if any.
  Coord GetIndent(int tab_stop) const;

  // Return original indent string.
  std::wstring GetIndentStr() const;

  // Return the length of the original indent string.
  Coord GetIndentStrLength() const;

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

  void AddLexElem(const LexElem& lex_elem) {
    AddLexElem(lex_elem.off, lex_elem.len, lex_elem.lex);
  }

  void AddLexElem(size_t off, size_t len, Lex lex);

  // Return true if there was any lex element.
  bool ClearLexElems();

  const std::list<LexElem*>& lex_elems() const {
    return lex_elems_;
  }

  // Return the lex elements inside the given char range.
  std::list<const LexElem*> lex_elems(const CharRange& char_range) const;

  // Get the lex at the given off.
  Lex GetLex(Coord off) const;

  // Return true if this line only has spaces.
  bool IsSpaceOnly() const;

  // Return true if this line only has comments.
  bool IsCommentOnly() const;

  // Return true if it's comment at the given off.
  bool IsComment(Coord off) const;

  // Return true if it's string at the given off.
  bool IsString(Coord off) const;

  // Return true if it's comment or string at the given off.
  bool IsCommentOrString(Coord off) const;

  void AddQuoteElem(Quote* quote, size_t off, size_t len, QuotePart part);

  // Return true if there was any quote element.
  bool ClearQuoteElems();

  Quote* UnendedQuote(bool multi_line) const;

  // Return true if this line ends the given quote.
  bool EndQuote(Quote* quote) const;

  const std::list<QuoteElem>& quote_elems() const {
    return quote_elems_;
  }

  bool GetQuoteElem(Coord off, const QuoteElem** start, const QuoteElem** end) const;

  const QuoteElem* FirstUnstartedQuoteEnd() const;
  const QuoteElem* LastUnendedQuoteStart() const;

  // Clear both lex elements and quote elements.
  // Return true if there was any lex or quote element.
  bool ClearLex();

  //----------------------------------------------------------------------------
  // Lua wrappers

  // Lua wrapper of Char.
  bool Lua_isChar(Coord off, char c) const;

  // Lua wrapper of FindLastChar.
  int Lua_findLastChar(char c, bool ignore_comment, int off = kInvCoord) const;

  // Lua CFunction wrapper of StartWith.
  // The parameters are different from the normal StartWith. It accepts
  // multiple strings, and the output parameter 'off' is returned as a
  // second value.
  // Call it in Lua like this:
  //   local ok, off = startWith(true, "str1", "str2", ...)
  int Lua_startWith(lua_State* L);

  // Lua CFunction wrapper of EndWith:
  //   bool EndWith(const std::wstring&, bool, bool, Coord*).
  int Lua_endWith(lua_State* L);

  // Lua wrapper of Equal:
  bool Lua_equal(const std::string& str,
                 bool ignore_comment,
                 bool ignore_space) const;

  // Lua wrapper of UnpairedLeftKey.
  Coord Lua_getUnpairedLeftKey(char l_key, char r_key, Coord off) const;

private:
  size_t id_;
  std::wstring data_;

  std::list<LexElem*> lex_elems_;

  std::list<QuoteElem> quote_elems_;

  // NOTE:
  // wxAny takes 24 bytes. So boost::any is prefered because it takes only
  // the same bytes as a pointer.
  boost::any extra_data_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_LINE_H_
