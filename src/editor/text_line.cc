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
  return data_.find_first_not_of(L" \t") == std::wstring::npos;
}

bool TextLine::StartWith(const std::wstring& str,
                         bool ignore_spaces,
                         Coord* off) const {
  size_t i = 0;

  if (ignore_spaces) {
    for (; i < data_.size() && IsSpace(data_[i]); ++i) {}
  }

  if (data_.size() < i + str.size()) {
    return false;
  }

  if (wcsncmp(&data_[i], &str[0], str.size()) == 0) {
    if (off != NULL) {
      *off = i;
    }
    return true;
  }

  return false;
}

//bool TextLine::EndWith(wchar_t c, bool ignore_spaces) const {
//  if (data_.empty()) {
//    return false;
//  }
//
//  size_t i = data_.size() - 1;
//
//  if (ignore_spaces) {
//    for (; i > 0 && IsSpace(data_[i]); --i) {}
//  }
//
//  return data_[i] == c;
//}

bool TextLine::EndWith(const std::wstring& str,
                       bool ignore_spaces,
                       Coord* off) const {
  if (data_.empty()) {
    return false;
  }

  size_t len = data_.size();

  if (ignore_spaces) {
    for (; len > 0 && IsSpace(data_[len-1]); --len) {}
  }

  if (len < str.size()) {
    return false;
  }

  size_t i = len - str.size();
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

Coord TextLine::TabbedLength(int tab_stop) const {
  return TabbedLineLength(tab_stop, data_);
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

void TextLine::AddQuoteInfo(Quote* quote,
                            size_t off,
                            size_t len,
                            Quote::Part part) {
  QuoteInfo qi = { quote, off, len, part };
  quote_infos_.push_back(qi);
}

Quote* TextLine::UnendedQuote(bool multi_line) const {
  if (!quote_infos_.empty()) {
    const QuoteInfo& qi = quote_infos_.back();
    if (qi.part != Quote::kEnd && multi_line == qi.quote->multi_line()) {
      return qi.quote;
    }
  }
  return NULL;
}

bool TextLine::EndsQuote(Quote* quote) const {
  std::list<QuoteInfo>::const_iterator it = quote_infos_.begin();
  for (; it != quote_infos_.end(); ++it) {
    const QuoteInfo& qi = *it;

    if (qi.quote == quote) {
      if (qi.part == Quote::kEnd) {
        return true;
      } else if (qi.part == Quote::kStart) {
        return false;
      }
    }
  }

  return false;
}

}  // namespace editor
}  // namespace jil
