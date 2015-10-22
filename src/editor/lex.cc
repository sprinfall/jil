#include "editor/lex.h"
#include "editor/util.h"

#if JIL_LEX_USE_RELITE
#include "editor/relite.h"
#endif  // JIL_LEX_USE_RELITE

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

Quote::Quote(Lex lex, const std::wstring& start, const std::wstring& end, int flags)
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

RegexQuote::RegexQuote(Lex lex, const std::wstring& start, const std::wstring& end, int flags)
    : Quote(lex, start, end, flags)
    , start_re_(NULL) {
  assert(!start_.empty());

#if JIL_LEX_USE_RELITE
  int re_flags = ignore_case_ ? relite::Regex::kIgnoreCase : 0;
  start_re_ = new relite::Regex(start_, re_flags);
#else
  if (start_[0] != L'^') {
    start_.insert(start_.begin(), L'^');
  }

  std::wregex::flag_type re_flags = std::wregex::ECMAScript;
  if (ignore_case_) {
    re_flags |= std::wregex::icase;
  }

  start_re_ = new std::wregex(start_, re_flags);
  #endif  // JIL_LEX_USE_RELITE
}

RegexQuote::~RegexQuote() {
  if (start_re_ != NULL) {
    delete start_re_;
  }

  ClearContainer(&quotes_);
}

size_t RegexQuote::MatchStart(const std::wstring& str, size_t off, std::wstring* concrete_end) const {
#if JIL_LEX_USE_RELITE
  if (!start_re_->valid()) {
    return off;
  }

  size_t match_off = start_re_->Match(str, off);
  if (match_off > off) {
    // TODO
    return match_off;
  }
#else
  // NOTE(20140718): Using member variable doesn't improve performance.
  std::match_results<std::wstring::const_iterator> m;
  std::regex_constants::match_flag_type flags = std::regex_constants::match_default;
  bool result = std::regex_search(str.begin() + off, str.end(), m, *start_re_, flags);
  if (result) {
    CreateConcreteEnd(str, m, concrete_end);
    return off + (m[0].second - m[0].first);
  }
#endif  // JIL_LEX_USE_RELITE

  return off;
}

#if JIL_LEX_USE_RELITE
// TODO
#else
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
#endif  // JIL_LEX_USE_RELITE

////////////////////////////////////////////////////////////////////////////////

Regex::Regex(const std::wstring& pattern, bool ignore_case)
    : pattern_(pattern), re_(NULL) {
  assert(!pattern.empty());

#if JIL_LEX_USE_RELITE
  int flags = ignore_case ? relite::Regex::kIgnoreCase : 0;
  re_ = new relite::Regex(pattern_, flags);

#else
  if (pattern_[0] != L'^') {
    pattern_.insert(pattern_.begin(), L'^');
  }

  std::wregex::flag_type re_flags = std::wregex::ECMAScript;
  if (ignore_case) {
    re_flags |= std::wregex::icase;
  }

  re_ = new std::wregex(pattern_, re_flags);
#endif  // JIL_LEX_USE_RELITE
}

Regex::~Regex() {
  if (re_ != NULL) {
    delete re_;
  }
}

size_t Regex::Match(const std::wstring& str, size_t off) const {
  if (re_ == NULL) {
    return off;
  }

  assert(off < str.length());

#if JIL_LEX_USE_RELITE
  if (!re_->valid()) {
    return off;
  }

  size_t match_off = re_->Match(str, off);
  return match_off;

#else
  std::match_results<std::wstring::const_iterator> m;
  std::regex_constants::match_flag_type flags = std::regex_constants::match_default;

  bool result = std::regex_search(str.begin() + off, str.end(), m, *re_, flags);
  if (!result) {
    return off;
  }

  size_t matched_length = std::distance(m[0].first, m[0].second);
  if (matched_length > 0) {
    return off + matched_length;
  }

  return off;

#endif  // JIL_LEX_USE_RELITE
}

}  // namespace editor
}  // namespace jil
