#include "editor/text_line.h"

extern "C" {
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

#include "editor/compile_config.h"
#include "editor/text_extent.h"
#include "editor/defs.h"
#include "editor/tab.h"
#include "editor/util.h"

namespace jil {
namespace editor {

//------------------------------------------------------------------------------

TextLine::TextLine(size_t id, const wchar_t* data, size_t len)
    : id_(id), data_(data, len), tabs_(0) {
  tabs_ = CountTabs(data_);
}

TextLine::TextLine(size_t id, const wchar_t* data)
    : id_(id), data_(data), tabs_(0) {
  tabs_ = CountTabs(data_);
}

TextLine::TextLine(size_t id, const std::wstring& data)
    : id_(id), data_(data), tabs_(0) {
  tabs_ = CountTabs(data_);
}

TextLine::~TextLine() {
  ClearContainer(&lex_elems_);
}

//------------------------------------------------------------------------------

Coord TextLine::TabbedLength(int tab_stop, Coord count) const {
  return TabbedLineLength(tab_stop, data_, count);
}

wchar_t TextLine::Char(Coord off) const {
  if (off >= Length()) {
    return LF;
  }
  return data_[off];
}

std::wstring TextLine::Sub(Coord off, Coord count) const {
  if (off >= Length()) {
    return L"";
  }

  if (count == kInvCoord) {
    return data_.substr(off, std::wstring::npos);
  }

  if (off + count > Length()) {
    return data_.substr(off, std::wstring::npos);
  }

  return data_.substr(off, count);
}


Coord TextLine::FindNonSpace(Coord off) const {
  assert(off >= 0);

  Coord len = Length();
  if (off >= len) {
    return len;
  }

  Coord i = off;
  for (; i < len && IsSpace(data_[i]); ++i) {}
  return i;
}

Coord TextLine::FindLastNonSpace(Coord off) const {
  if (off == kInvCoord) {
    off = Length();
  }

  Coord i = off - 1;
  for (; i >= 0 && IsSpace(data_[i]); --i) {}
  return i;  // Might be -1, i.e., kInvCoord.
}

Coord TextLine::FindLastChar(wchar_t c, bool ignore_comment, Coord off) const {
  if (off == kInvCoord) {
    off = Length();
  }

  Coord i = off - 1;
  for (; i >= 0; --i) {
    if (data_[i] == c) {
      if (ignore_comment && IsComment(i)) {
        continue;
      }
      break;
    }
  }

  return i;
}

bool TextLine::IsEmpty(bool ignore_space) const {
  if (ignore_space) {
    return data_.find_first_not_of(L" \t") == std::wstring::npos;
  } else {
    return data_.empty();
  }
}

bool TextLine::StartWith(wchar_t c,
                         bool ignore_comment,
                         bool ignore_space,
                         Coord* off) const {
  size_t i = 0;

  if (ignore_space && !IsSpace(c)) {
    for (; i < data_.size() && IsSpace(data_[i]); ++i) {}
  }

  if (i == data_.size()) {
    return false;
  }

  if (data_[i] == c) {
    if (ignore_comment && IsComment(i)) {
      return false;
    } else {
      if (off != NULL) {
        *off = CoordCast(i);
      }
      return true;
    }
  }

  return false;
}

bool TextLine::StartWith(const std::wstring& str,
                         bool ignore_comment,
                         bool ignore_space,
                         Coord* off) const {
  if (str.size() == 1) {
    return StartWith(str[0], ignore_comment, ignore_space, off);
  }

  size_t i = 0;

  if (ignore_space && !IsSpace(str[0])) {
    for (; i < data_.size() && IsSpace(data_[i]); ++i) {}
  }

  if (data_.size() < i + str.size()) {
    return false;
  }

  if (wcsncmp(&data_[i], &str[0], str.size()) == 0) {
    if (ignore_comment && IsComment(i)) {
      return false;
    } else {
      if (off != NULL) {
        *off = CoordCast(i);
      }
      return true;
    }
  }

  return false;
}

bool TextLine::EndWith(wchar_t c,
                       bool ignore_comment,
                       bool ignore_space,
                       Coord* off) const {
  if (data_.empty()) {
    return false;
  }

  Coord len = CoordCast(data_.size());

  bool do_ignore_spaces = ignore_space && !IsSpace(c);

  if (ignore_comment) {
    std::list<LexElem*>::const_reverse_iterator it = lex_elems_.rbegin();
    for (; it != lex_elems_.rend(); ++it) {
      if (do_ignore_spaces) {
        for (; len > 0 && IsSpace(data_[len-1]); --len) {}
      }

      const LexElem* le = *it;
      if (le->off + le->len >= len && le->lex == kLexComment) {
        len = le->off;
      } else {
        break;
      }
    }
  }

  if (do_ignore_spaces) {
    for (; len > 0 && IsSpace(data_[len-1]); --len) {}
  }

  if (len == 0) {
    return false;
  }

  Coord i = len - 1;
  if (data_[i] == c) {
    if (off != NULL) {
      *off = i;
    }
    return true;
  }

  return false;
}

bool TextLine::EndWith(const std::wstring& str,
                       bool ignore_comment,
                       bool ignore_space,
                       Coord* off) const {
  if (str.size() == 1) {
    return EndWith(str[0], ignore_comment, ignore_space, off);
  }

  if (data_.empty()) {
    return false;
  }

  Coord len = CoordCast(data_.size());

  bool do_ignore_spaces = ignore_space && !IsSpace(str[str.size()-1]);

  if (ignore_comment) {
    std::list<LexElem*>::const_reverse_iterator it = lex_elems_.rbegin();
    for (; it != lex_elems_.rend(); ++it) {
      if (do_ignore_spaces) {
        for (; len > 0 && IsSpace(data_[len-1]); --len) {}
      }

      const LexElem* le = *it;
      if (le->off + le->len >= len && le->lex == kLexComment) {
        len = le->off;
      } else {
        break;
      }
    }
  }

  if (do_ignore_spaces) {
    for (; len > 0 && IsSpace(data_[len-1]); --len) {}
  }

  if (len < CoordCast(str.size())) {
    return false;
  }

  Coord i = len - CoordCast(str.size());
  if (wcsncmp(&data_[i], &str[0], str.size()) == 0) {
    if (off != NULL) {
      *off = i;
    }
    return true;
  }

  return false;
}

bool TextLine::Equal(const std::wstring& str,
                     bool ignore_comment,
                     bool ignore_space) const {
  Coord off1 = kInvCoord;
  if (!StartWith(str, ignore_comment, ignore_space, &off1)) {
    return false;
  }

  Coord off2 = kInvCoord;
  if (!EndWith(str, ignore_comment, ignore_space, &off2)) {
    return false;
  }

  return off1 == off2;
}

bool TextLine::IsEolEscaped(bool no_comment_or_string) const {
  if (data_.empty()) {
    return false;
  }
  size_t off = data_.size() - 1;
  if (IsUnescapedBackSlash(data_, off)) {
    if (no_comment_or_string) {
      if (IsCommentOrString(off)) {
        return false;
      }
    }
    return true;
  }
  return false;
}

Coord TextLine::UnpairedLeftKey(wchar_t l_key, wchar_t r_key, Coord off) const {
  if (off == kInvCoord) {
    off = Length();
  }

  int counter = 0;

  for (Coord x = off - 1; x >= 0; --x) {
    if (Char(x) == r_key && GetLex(x).IsEmpty()) {  // Embedded pair.
      ++counter;
    } else if (Char(x) == l_key && GetLex(x).IsEmpty()) {
      if (counter == 0) {
        return x;
      } else {
        --counter;
      }
    }
  }

  return kInvCoord;
}

Coord TextLine::GetIndent(int tab_stop) const {
  Coord spaces = 0;

  const Coord length = Length();
  Coord i = 0;

  for (; i < length && IsSpace(data_[i]); ++i) {
    if (data_[i] == kSpaceChar) {
      ++spaces;
    } else if (data_[i] == kTabChar) {
      spaces += tab_stop - (spaces % tab_stop);
    } else {
      break;
    }
  }

  return spaces;
}

std::wstring TextLine::GetIndentStr() const {
  Coord i = FindNonSpace(0);

  if (i == 0) {
    return L"";
  } else if (i == Length()) {
    return data_;
  } else {
    return data_.substr(0, i);
  }
}

Coord TextLine::GetIndentStrLength() const {
  return FindNonSpace(0);
}

IndentProp TextLine::GetIndentProp() const {
  IndentProp prop;
  prop.length = GetIndentStrLength();

  if (prop.length == 0) {
    prop.type = kNoIndent;
    return prop;
  }

  prop.type = data_[0] == kTabChar ? kTabIndent : kSpaceIndent;
  for (int i = 1; i < prop.length; ++i) {
    if (data_[i] != data_[0]) {
      prop.type = kMixedIndent;
      break;
    }
  }

  return prop;
}

//------------------------------------------------------------------------------

void TextLine::InsertChar(Coord off, wchar_t c) {
  data_.insert(off, 1, c);

  if (c == kTabChar) {
    ++tabs_;
  }
}

void TextLine::DeleteChar(Coord off, wchar_t* c) {
  if (data_[off] == kTabChar) {
    --tabs_;
  }

  if (c != NULL) {
    *c = data_[off];
  }

  data_.erase(off, 1);
}

void TextLine::InsertString(Coord off, const std::wstring& str) {
  data_.insert(off, str);

  tabs_ += CountTabs(str);
}

void TextLine::DeleteString(Coord off, Coord count, std::wstring* str) {
  if (off >= Length()) {
    return;
  }

  if (str != NULL) {
    *str = Sub(off, count);
  }

  if (count == kInvCoord) {
    tabs_ -= CountTabs(data_, off);
    data_.erase(off, std::wstring::npos);
    return;
  }

  if (off + count > Length()) {
    tabs_ -= CountTabs(data_, off);
    data_.erase(off, std::wstring::npos);
    return;
  }

  tabs_ -= CountTabs(data_, off, count);
  data_.erase(off, count);
}

void TextLine::Clear(std::wstring* line_data) {
  if (line_data != NULL) {
    line_data->clear();
    line_data->swap(data_);
  } else {
    data_.clear();
  }

  tabs_ = 0;
}

void TextLine::Append(const std::wstring& str) {
  data_.append(str);

  tabs_ += CountTabs(str);
}

TextLine* TextLine::Split(Coord off, size_t line_id) {
  assert(off <= Length());

  TextLine* new_line = NULL;
  if (off == Length()) {
    new_line = new TextLine(line_id, L"");
  } else {
    new_line = new TextLine(line_id, &data_[off], data_.size() - off);
    tabs_ -= CountTabs(data_, off);
    data_.erase(off);
  }

  return new_line;
}

//------------------------------------------------------------------------------
// Lex

void TextLine::AddLexElem(size_t off, size_t len, Lex lex) {
  // Merge continuous same lex elements.
  if (!lex_elems_.empty()) {
    LexElem* prev_le = lex_elems_.back();
    if (lex == prev_le->lex && off == (prev_le->off + prev_le->len)) {
      prev_le->len += len;
      return;
    }
  }
  lex_elems_.push_back(new LexElem(off, len, lex));
}

bool TextLine::ClearLexElems() {
  if (lex_elems_.empty()) {
    return false;
  }
  ClearContainer(&lex_elems_);
  return true;
}

std::list<const LexElem*> TextLine::lex_elems(const CharRange& char_range) const {
  std::list<const LexElem*> range_lex_elements;

  CharRange adjusted_char_range = char_range;
  if (adjusted_char_range.end() == kInvCoord) {
    adjusted_char_range.set_end(Length());
  }

  std::list<LexElem*>::const_iterator it = lex_elems_.begin();
  for (; it != lex_elems_.end(); ++it) {
    const LexElem* le = *it;

    if (le->off >= adjusted_char_range.end()) {
      break;
    }

    Coord le_end = le->off + le->len;
    if (le_end <= adjusted_char_range.begin()) {
      continue;
    }

    range_lex_elements.push_back(le);
  }

  return range_lex_elements;
}

Lex TextLine::GetLex(Coord off) const {
  if (off >= Length()) {
    if (!quote_elems_.empty()) {
      const QuoteElem& qe = quote_elems_.back();
      if (qe.part != kQuoteEnd && (qe.off + qe.len) == Length()) {
        return qe.quote->lex();
      }
    }
    return Lex();
  }

  std::list<LexElem*>::const_iterator it = lex_elems_.begin();
  for (; it != lex_elems_.end(); ++it) {
    const LexElem* le = *it;

    if (off < le->off) {
      break;
    }

    if (off < le->off + le->len) {
      return le->lex;
    }
  }

  return Lex();
}

bool TextLine::IsSpaceOnly() const {
  if (data_.empty()) {
    return false;
  }

  for (size_t i = 0; i < data_.size(); ++i) {
    if (!IsSpace(data_[i])) {
      return false;
    }
  }

  return true;
}

bool TextLine::IsComment(Coord off) const {
  return (GetLex(off).major() == kLexComment);
}

bool TextLine::IsString(Coord off) const {
  return (GetLex(off) == Lex(kLexConstant, kLexConstantString));
}

bool TextLine::IsCommentOrString(Coord off) const {
  Lex lex = GetLex(off);
  if (lex.major() == kLexComment) {
    return true;
  }
  return (lex == Lex(kLexConstant, kLexConstantString));
}

bool TextLine::IsCommentOnly() const {
  return IsLexOnly(Lex(kLexComment));
}

bool TextLine::IsStringOnly() const {
  return IsLexOnly(Lex(kLexConstant, kLexConstantString));
}

bool TextLine::IsLexOnly(Lex lex) const {
  if (lex_elems_.empty()) {
    return false;
  }

  Coord i = 0;

  std::list<LexElem*>::const_iterator it = lex_elems_.begin();
  for (; it != lex_elems_.end(); ++it) {
    const LexElem* le = *it;

    if (le->lex != lex) {
      return false;
    }

    if (i < le->off) {
      // Line piece (spaces, operators, plain-text, etc.) with no lex.
      for (; i < le->off; ++i) {
        if (!IsSpace(data_[i])) {
          return false;
        }
      }
    }

    i = le->off + le->len;
  }

  // Line piece after the last lex element.
  if (i < Length()) {
    // Line piece (spaces, operators, plain-text, etc.) with no lex element.
    for (; i < Length(); ++i) {
      if (!IsSpace(data_[i])) {
        return false;
      }
    }
  }

  return true;
}

void TextLine::AddQuoteElem(Quote* quote, size_t off, size_t len, QuotePart part) {
  QuoteElem qe = { quote, CoordCast(off), CoordCast(len), part };
  quote_elems_.push_back(qe);
}

bool TextLine::ClearQuoteElems() {
  if (quote_elems_.empty()) {
    return false;
  }
  quote_elems_.clear();
  return true;
}

Quote* TextLine::UnendedQuote(bool multi_line) const {
  if (!quote_elems_.empty()) {
    const QuoteElem& qe = quote_elems_.back();
    if (qe.part != kQuoteEnd && multi_line == qe.quote->multi_line()) {
      return qe.quote;
    }
  }
  return NULL;
}

bool TextLine::EndQuote(Quote* quote) const {
  std::list<QuoteElem>::const_iterator it = quote_elems_.begin();
  for (; it != quote_elems_.end(); ++it) {
    const QuoteElem& qe = *it;

    if (qe.quote == quote) {
      if (qe.part == kQuoteEnd) {
        return true;
      } else if (qe.part == kQuoteStart) {
        return false;
      }
    }
  }

  return false;
}

bool TextLine::GetQuoteElem(Coord off, const QuoteElem** start, const QuoteElem** end) const {
  std::list<QuoteElem>::const_iterator it = quote_elems_.begin();

  bool found = false;

  for (; it != quote_elems_.end(); ++it) {
    const QuoteElem& qe = *it;

    if (off < qe.off) {
      break;
    }

    if (off <= qe.off + qe.len) {
      found = true;
      break;
    }
  }

  if (found) {
    const QuoteElem& qe = *it;

    if (qe.part == kQuoteStart) {
      *start = &qe;

      ++it;
      if (it != quote_elems_.end()) {
        if (it->part == kQuoteBody) {
          ++it;
          if (it != quote_elems_.end()) {
            *end = &(*it);
          }
        } else {
          *end = &(*it);
        }
      }
    } else if (qe.part == kQuoteEnd) {
      *end = &qe;

      if (it != quote_elems_.begin()) {
        --it;
        if (it->part == kQuoteBody) {
          if (it != quote_elems_.begin()) {
            --it;
            *start = &(*it);
          }
        } else {
          *start = &(*it);
        }
      }
    } else if (qe.part == kQuoteBody) {
      if (it != quote_elems_.begin()) {
        std::list<QuoteElem>::const_iterator prev_it = it;
        --prev_it;
        *start = &(*prev_it);
      }

      ++it;
      if (it != quote_elems_.end()) {
        *end = &(*it);
      }
    }
  }

  return found;
}

const QuoteElem* TextLine::FirstUnstartedQuoteEnd() const {
  std::list<QuoteElem>::const_iterator it = quote_elems_.begin();

  for (; it != quote_elems_.end(); ++it) {
    if (it->part == kQuoteEnd) {
      return &(*it);
    } else if (it->part == kQuoteStart) {
      // Embedded quote is not supported.
      break;
    }
  }

  return NULL;
}

const QuoteElem* TextLine::LastUnendedQuoteStart() const {
  std::list<QuoteElem>::const_reverse_iterator it = quote_elems_.rbegin();

  for (; it != quote_elems_.rend(); ++it) {
    if (it->part == kQuoteStart) {
      return &(*it);
    } else if (it->part == kQuoteEnd) {
      // Embedded quote is not supported.
      break;
    }
  }

  return NULL;
}

bool TextLine::ClearLex() {
  bool result1 = ClearLexElems();
  bool result2 = ClearQuoteElems();
  return result1 || result2;
}

//------------------------------------------------------------------------------
// Lua wrappers

bool TextLine::Lua_isChar(Coord off, char c) const {
  return (c == Char(off));
}

int TextLine::Lua_findLastChar(char c, bool ignore_comment, int off) const {
  return FindLastChar(wchar_t(c), ignore_comment, off);
}

int TextLine::Lua_startWith(lua_State* L) {
  int n = lua_gettop(L);  // Number of arguments

  if (n < 4) {
    luaL_error(L, "incorrect argument");
  }

  if (lua_type(L, 2) != LUA_TBOOLEAN || lua_type(L, 3) != LUA_TBOOLEAN) {
    luaL_error(L, "incorrect argument");
  }

  bool ignore_comment = lua_toboolean(L, 2) != 0;
  bool ignore_space = lua_toboolean(L, 3) != 0;

  bool result = false;
  int off = 0;

  for (int i = 4; i <= n; ++i) {
    if (lua_type(L, i) != LUA_TSTRING) {
      break;
    }
    const char* cstr = lua_tostring(L, i);
    std::wstring str(cstr, cstr + strlen(cstr));
    if (StartWith(str, ignore_comment, ignore_space, &off)) {
      result = true;
      break;
    }
  }

  luabridge::push(L, result);
  luabridge::push(L, off);
  return 2;
}

int TextLine::Lua_endWith(lua_State* L) {
  int n = lua_gettop(L);  // Number of arguments

  if (n < 4) {
    luaL_error(L, "incorrect argument");
  }

  if (lua_type(L, 2) != LUA_TBOOLEAN || lua_type(L, 3) != LUA_TBOOLEAN) {
    luaL_error(L, "incorrect argument");
  }

  bool ignore_comment = lua_toboolean(L, 2) != 0;
  bool ignore_space = lua_toboolean(L, 3) != 0;

  bool result = false;
  int off = 0;

  for (int i = 4; i <= n; ++i) {
    if (lua_type(L, i) != LUA_TSTRING) {
      break;
    }
    const char* cstr = lua_tostring(L, i);
    std::wstring str(cstr, cstr + strlen(cstr));
    if (EndWith(str, ignore_comment, ignore_space, &off)) {
      result = true;
      break;
    }
  }

  luabridge::push(L, result);
  luabridge::push(L, off);
  return 2;
}

bool TextLine::Lua_equal(const std::string& str,
                         bool ignore_comment,
                         bool ignore_space) const {
  std::wstring wstr(str.begin(), str.end());
  return Equal(wstr, ignore_comment, ignore_space);
}

Coord TextLine::Lua_getUnpairedLeftKey(char l_key, char r_key, Coord off) const {
  return UnpairedLeftKey((wchar_t)l_key, (wchar_t)r_key, off);
}

int TextLine::CountTabs(const std::wstring& str, size_t off, size_t count) const {
  int tabs = 0;
  size_t end = count == std::wstring::npos ? str.size() : (off + count);
  for (size_t i = off; i < end; ++i) {
    if (str[i] == kTabChar) {
      ++tabs;
    }
  }
  return tabs;
}

}  // namespace editor
}  // namespace jil
