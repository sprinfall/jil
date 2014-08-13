#include "editor/lex.h"
#include "boost/regex.hpp"
#include "editor/util.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

typedef boost::match_results<std::wstring::const_iterator> regex_match_results;

////////////////////////////////////////////////////////////////////////////////

Quote::Quote()
    : lex_(kLexNone), flags_(0), ignore_case_(false) {
}

Quote::Quote(Lex lex,
             const std::wstring& start,
             const std::wstring& end,
             int flags)
  : lex_(lex), start_(start), end_(end), flags_(flags) {
}

Quote::~Quote() {
}

void Quote::Set(Lex lex,
                const std::wstring& start,
                const std::wstring& end,
                int flags) {
  lex_ = lex;
  start_ = start;
  end_ = end;
  flags_ = flags;
}

size_t Quote::MatchStart(const std::wstring& str, size_t off) const {
  WcsNCmp cmp = ignore_case_ ? wcsnicmp : wcsncmp;
  if (SubStringEquals(str, off, start_, cmp)) {
    return off + start_.length();
  }
  return off;
}

////////////////////////////////////////////////////////////////////////////////

#if JIL_LEX_REGEX_QUOTE_START

size_t RegexQuote::MatchStart(const std::wstring& str, size_t off) const {
  if (start_re_ == NULL) {
    start_re_ = new wregex(start_);
  }

  const int flags = boost::regex_constants::match_default;

  // NOTE(20140718, performance):
  //   Using member variable doesn't improve performance.
  regex_match_results m;

  std::wstring::const_iterator begin(str.begin() + off);
  std::wstring::const_iterator end(str.end());

  bool result = boost::regex_search(begin, end, m, *start_re_, flag);
  if (result) {
    return off + (m[0].second - m[0].first);
  }

  return off;
}

RegexQuote::RegexQuote()
    : start_re_(NULL) {
}

RegexQuote::~RegexQuote() {
  if (start_re_ != NULL) {
    delete start_re_;
  }
}

#endif  // JIL_LEX_REGEX_QUOTE_START

////////////////////////////////////////////////////////////////////////////////

Regex::Regex()
    : re_(NULL) {
}

Regex::~Regex() {
  if (re_ != NULL) {
    delete re_;
  }
}

void Regex::set_pattern(const std::wstring& pattern, bool ignore_case) {
  assert(!pattern.empty());

  pattern_ = pattern;
  if (pattern_[0] != L'^') {
    pattern_.insert(pattern_.begin(), L'^');
  }

  const int flag = boost::regex_constants::icase;

  if (re_ == NULL) {
    re_ = new boost::wregex(pattern_, flag);
  } else {
    re_->set_expression(pattern_, flag);
  }
}

size_t Regex::Match(const std::wstring& str, size_t off) const {
  if (re_ == NULL) {
    return off;
  }

  assert(off < str.length());

  regex_match_results m;
  std::wstring::const_iterator begin(str.begin() + off);
  std::wstring::const_iterator end(str.end());

  bool result = boost::regex_search(begin, end, m, *re_, boost::match_default);
  if (!result) {
    return off;
  }

  size_t matched_length = std::distance(m[0].first, m[0].second);
  if (matched_length > 0) {
    return off + matched_length;
  }
  return off;
}

}  // namespace editor
}  // namespace jil
