#include "editor/lex.h"
#include "editor/util.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

Quote::Quote(Lex lex,
             const std::wstring& start,
             const std::wstring& end,
             int flags)
    : lex_(lex), start_(start), end_(end), flags_(flags)
    , ignore_case_(false) {
}

Quote::~Quote() {
}

size_t Quote::MatchStart(const std::wstring& str, size_t off) const {
  WcsNCmp cmp = ignore_case_ ? wcsnicmp : wcsncmp;
  if (SubStringEquals(str, off, start_, cmp)) {
    return off + start_.length();
  }
  return off;
}

////////////////////////////////////////////////////////////////////////////////

RegexQuote::RegexQuote(Lex lex,
                       const std::wstring& start,
                       const std::wstring& end,
                       int flags)
    : Quote(lex, start, end, flags)
    , start_re_(NULL) {
  assert(!start_.empty());

  if (start_[0] != L'^') {
    start_.insert(start_.begin(), L'^');
  }

  int flag = 0;
  if (ignore_case_) {
    flag = boost::regex_constants::icase;
  }

  if (start_re_ == NULL) {
    start_re_ = new boost::wregex(start_, flag);
  } else {
    start_re_->set_expression(start_, flag);
  }
}

RegexQuote::~RegexQuote() {
  if (start_re_ != NULL) {
    delete start_re_;
  }

  ClearContainer(&quotes_);
}

size_t RegexQuote::MatchStart(const std::wstring& str,
                              size_t off,
                              std::wstring* concrete_end) const {
  // NOTE(20140718): Using member variable doesn't improve performance.
  boost::match_results<std::wstring::const_iterator> m;

  boost::regex_constants::match_flags flags =
      boost::regex_constants::match_default;

  bool result = boost::regex_search(str.begin() + off,
                                    str.end(),
                                    m,
                                    *start_re_,
                                    flags);
  if (result) {
    CreateConcreteEnd(str, m, concrete_end);
    return off + (m[0].second - m[0].first);
  }

  return off;
}

bool RegexQuote::CreateConcreteEnd(const std::wstring& str,
                                   RegexQuote::MatchResult& m,
                                   std::wstring* concrete_end) const {
  if (end_.empty()) {
    return true;
  }

  // NOTE: only support back reference "\1".
  size_t br_pos = end_.find(L"\\1");
  if (br_pos == std::wstring::npos) {
    return true;
  }

  if (m.size() <= 1) {
    return false;  // No sub match result.
  }

  std::wstring sub;
  size_t sub_size = std::distance(m[1].first, m[1].second);
  if (sub_size > 0) {
    size_t sub_off = std::distance(str.begin(), m[1].first);
    sub = str.substr(sub_off, sub_size);
  }  // else: The sub matched string is empty.

  *concrete_end = end_;
  concrete_end->replace(br_pos, 2, sub);

  return true;
}


////////////////////////////////////////////////////////////////////////////////

Regex::Regex() : re_(NULL) {
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

  int flag = 0;
  if (ignore_case) {
    flag = boost::regex_constants::icase;
  }

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

  boost::match_results<std::wstring::const_iterator> m;

  boost::regex_constants::match_flags flags =
      boost::regex_constants::match_default;

  bool result = boost::regex_search(str.begin() + off,
                                    str.end(),
                                    m,
                                    *re_,
                                    flags);
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
