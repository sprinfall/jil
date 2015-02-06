#include "editor/text_line.h"
#include "editor/compile_config.h"
#include "editor/text_extent.h"
#include "editor/defs.h"
#include "editor/tab.h"
#include "editor/util.h"

namespace jil {
namespace editor {

TextLine::TextLine(size_t id, const wchar_t* data, size_t len)
    : id_(id), data_(data, len) {
}

TextLine::TextLine(size_t id, const wchar_t* data)
    : id_(id), data_(data) {
}

TextLine::TextLine(size_t id, const std::wstring& data)
    : id_(id), data_(data) {
}

TextLine::~TextLine() {
  ClearContainer(&lex_elements_);
}

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

  if (count == kInvalidCoord) {
    return data_.substr(off, std::wstring::npos);
  }

  if (off + count > Length()) {
    return data_.substr(off, std::wstring::npos);
  }

  return data_.substr(off, count);
}

bool TextLine::IsEmpty(bool ignore_spaces) const {
  if (ignore_spaces) {
    return data_.find_first_not_of(L" \t") == std::wstring::npos;
  } else {
    return data_.empty();
  }
}

bool TextLine::StartWith(wchar_t c,
                         bool ignore_spaces,
                         Coord* off) const {
  size_t i = 0;

  if (ignore_spaces && !IsSpace(c)) {
    for (; i < data_.size() && IsSpace(data_[i]); ++i) {}
  }

  if (i == data_.size()) {
    return false;
  }

  if (data_[i] == c) {
    if (off != NULL) {
      *off = CoordCast(i);
    }
    return true;
  }

  return false;
}

bool TextLine::StartWith(const std::wstring& str,
                         bool ignore_spaces,
                         Coord* off) const {
  if (str.size() == 1) {
    return StartWith(str[0], ignore_spaces, off);
  }

  size_t i = 0;

  if (ignore_spaces && !IsSpace(str[0])) {
    for (; i < data_.size() && IsSpace(data_[i]); ++i) {}
  }

  if (data_.size() < i + str.size()) {
    return false;
  }

  if (wcsncmp(&data_[i], &str[0], str.size()) == 0) {
    if (off != NULL) {
      *off = CoordCast(i);
    }
    return true;
  }

  return false;
}

bool TextLine::EndWith(wchar_t c,
                       bool ignore_comments,
                       bool ignore_spaces,
                       Coord* off) const {
  if (data_.empty()) {
    return false;
  }

  Coord len = CoordCast(data_.size());

  bool do_ignore_spaces = ignore_spaces && !IsSpace(c);

  if (ignore_comments) {
    std::list<LexElement*>::const_reverse_iterator it = lex_elements_.rbegin();
    for (; it != lex_elements_.rend(); ++it) {
      if (do_ignore_spaces) {
        for (; len > 0 && IsSpace(data_[len-1]); --len) {}
      }

      const LexElement* le = *it;
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
                       bool ignore_comments,
                       bool ignore_spaces,
                       Coord* off) const {
  if (str.size() == 1) {
    return EndWith(str[0], ignore_comments, ignore_spaces, off);
  }

  if (data_.empty()) {
    return false;
  }

  Coord len = CoordCast(data_.size());

  bool do_ignore_spaces = ignore_spaces && !IsSpace(str[str.size()-1]);

  if (ignore_comments) {
    std::list<LexElement*>::const_reverse_iterator it = lex_elements_.rbegin();
    for (; it != lex_elements_.rend(); ++it) {
      if (do_ignore_spaces) {
        for (; len > 0 && IsSpace(data_[len-1]); --len) {}
      }

      const LexElement* le = *it;
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

Coord TextLine::FirstNonSpaceChar(Coord off) const {
  assert(off >= 0);

  Coord len = Length();
  if (off >= len) {
    return len;
  }

  Coord i = off;
  for (; i < len && IsSpace(data_[i]); ++i) {}
  return i;
}

Coord TextLine::LastNonSpaceChar(Coord off) const {
  if (off == kInvalidCoord) {
    off = Length();
  }

  Coord i = off - 1;
  for (; i >= 0 && IsSpace(data_[i]); --i) {}
  return i;  // Might be -1, i.e., kInvalidCoord.
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
  Coord i = FirstNonSpaceChar();

  if (i == 0) {
    return L"";
  } else if (i == Length()) {
    return data_;
  } else {
    return data_.substr(0, i);
  }
}

void TextLine::InsertChar(Coord off, wchar_t c) {
  data_.insert(off, 1, c);
}

void TextLine::DeleteChar(Coord off, wchar_t* c) {
  if (c != NULL) {
    *c = data_[off];
  }
  data_.erase(off, 1);
}

void TextLine::InsertString(Coord off, const std::wstring& str) {
  data_.insert(off, str);
}

void TextLine::DeleteString(Coord off, Coord count, std::wstring* str) {
  if (off >= Length()) {
    return;
  }

  if (str != NULL) {
    *str = Sub(off, count);
  }

  if (count == kInvalidCoord) {
    data_.erase(off, std::wstring::npos);
    return;
  }

  if (off + count > Length()) {
    data_.erase(off, std::wstring::npos);
    return;
  }

  data_.erase(off, count);
}

void TextLine::Clear(std::wstring* line_data) {
  if (line_data != NULL) {
    line_data->clear();
    line_data->swap(data_);
  } else {
    data_.clear();
  }
}

void TextLine::Append(const std::wstring& str) {
  data_.append(str);
}

TextLine* TextLine::Split(Coord off, size_t line_id) {
  assert(off <= Length());

  TextLine* new_line = NULL;
  if (off == Length()) {
    new_line = new TextLine(line_id, L"");
  } else {
    new_line = new TextLine(line_id, &data_[off], data_.size() - off);
    data_.erase(off);
  }

  return new_line;
}

//------------------------------------------------------------------------------
// Lex

void TextLine::AddLexElement(size_t off, size_t len, Lex lex) {
  // Merge continuous same lex elements.
  if (!lex_elements_.empty()) {
    LexElement* prev_le = lex_elements_.back();
    if (lex == prev_le->lex && off == (prev_le->off + prev_le->len)) {
      prev_le->len += len;
      return;
    }
  }
  lex_elements_.push_back(new LexElement(off, len, lex));
}

void TextLine::ClearLexElements() {
  ClearContainer(&lex_elements_);
}

std::list<const LexElement*> TextLine::lex_elements(
    const CharRange& char_range) const {
  std::list<const LexElement*> range_lex_elements;

  CharRange adjusted_char_range = char_range;
  if (adjusted_char_range.end() == kInvalidCoord) {
    adjusted_char_range.set_end(Length());
  }

  std::list<LexElement*>::const_iterator it = lex_elements_.begin();
  for (; it != lex_elements_.end(); ++it) {
    const LexElement* le = *it;

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
  std::list<LexElement*>::const_iterator it = lex_elements_.begin();
  for (; it != lex_elements_.end(); ++it) {
    const LexElement* le = *it;

    if (off < le->off) {
      break;
    }

    if (off < le->off + le->len) {
      return le->lex;
    }
  }

  return Lex();
}

bool TextLine::SpacesOnly() const {
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

bool TextLine::CommentsOnly() const {
  if (lex_elements_.empty()) {
    return false;
  }

  Coord i = 0;

  std::list<LexElement*>::const_iterator it = lex_elements_.begin();
  for (; it != lex_elements_.end(); ++it) {
    const LexElement* le = *it;

    if (le->lex != kLexComment) {
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

void TextLine::AddQuoteInfo(Quote* quote,
                            size_t off,
                            size_t len,
                            QuotePart part) {
  QuoteInfo qi = { quote, off, len, part };
  quote_infos_.push_back(qi);
}

Quote* TextLine::UnendedQuote(bool multi_line) const {
  if (!quote_infos_.empty()) {
    const QuoteInfo& qi = quote_infos_.back();
    if (qi.part != kQuoteEnd && multi_line == qi.quote->multi_line()) {
      return qi.quote;
    }
  }
  return NULL;
}

bool TextLine::EndQuote(Quote* quote) const {
  std::list<QuoteInfo>::const_iterator it = quote_infos_.begin();
  for (; it != quote_infos_.end(); ++it) {
    const QuoteInfo& qi = *it;

    if (qi.quote == quote) {
      if (qi.part == kQuoteEnd) {
        return true;
      } else if (qi.part == kQuoteStart) {
        return false;
      }
    }
  }

  return false;
}

}  // namespace editor
}  // namespace jil
