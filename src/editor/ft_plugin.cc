#include "editor/ft_plugin.h"
#if !JIL_MATCH_WORD_WITH_HASH
#include <algorithm>
#endif
#include "boost/regex.hpp"
#include "editor/lex.h"
#include "editor/util.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

// Compare operators for sorting word lex pairs.
inline bool operator<(const WordLexPair& lhs, const WordLexPair& rhs) {
  return lhs.first < rhs.first;
}
inline bool operator>(const WordLexPair& lhs, const WordLexPair& rhs) {
  return lhs.first > rhs.first;
}
inline bool operator==(const WordLexPair& lhs, const WordLexPair& rhs) {
  return lhs.first == rhs.first;
}
inline bool operator<=(const WordLexPair& lhs, const WordLexPair& rhs) {
  return lhs.first <= rhs.first;
}

////////////////////////////////////////////////////////////////////////////////

FtPlugin::FtPlugin(const FileType& file_type)
    : file_type_(file_type)
    , ignore_case_(false) {
  wcsncmp_ = wcsncmp;
  operators_ = L"!@#%^&*()+-=\\|/?[]{}<>,.;:'\"`~";
  delimiters_ = operators_ + L" \t";
}

FtPlugin::~FtPlugin() {
  ClearContainer(&quotes_);
  ClearContainer(&regexs_);
}

//------------------------------------------------------------------------------

bool FtPlugin::IsOperator(wchar_t c) const {
  return operators_.find(c) != std::wstring::npos;
}

bool FtPlugin::IsDelimiter(wchar_t c) const {
  return delimiters_.find(c) != std::wstring::npos;
}

//------------------------------------------------------------------------------

void FtPlugin::SetIgnoreCase(bool ignore_case) {
  ignore_case_ = ignore_case;
  wcsncmp_ = ignore_case_ ? wcsnicmp : wcsncmp;
}

void FtPlugin::AddAnyof(Lex lex, const std::wstring& joined_words) {
  size_t p = 0;

  for (size_t i = 0; i < joined_words.size(); ++i) {
    if (joined_words[i] == L' ') {
      if (i > p) {
#if JIL_MATCH_WORD_WITH_HASH
        anyofs_[joined_words.substr(p, i - p)] = lex;
#else
        anyofs_.push_back(
            std::make_pair(joined_words.substr(p, i - p), lex));
#endif
      }

      p = i + 1;
    }
  }

  if (p < joined_words.size()) {
#if JIL_MATCH_WORD_WITH_HASH
    anyofs_[joined_words.substr(p)] = lex;
#else
    anyofs_.push_back(std::make_pair(joined_words.substr(p), lex));
#endif
  }
}

#if !JIL_MATCH_WORD_WITH_HASH

void FtPlugin::SortAnyof() {
  anyofs_.shrink_to_fit();
  std::sort(anyofs_.begin(), anyofs_.end());
}

#endif  // !JIL_MATCH_WORD_WITH_HASH

void FtPlugin::AddQuote(Quote* quote) {
  quote->set_ignore_case(ignore_case_);
  quotes_.push_back(quote);
}

void FtPlugin::AddRegex(Lex lex, const std::wstring& pattern) {
  Regex* regex = new Regex;
  regex->set_lex(lex);
  regex->set_pattern(pattern, ignore_case_);
  regexs_.push_back(regex);
}

void FtPlugin::AddPrefix(Lex lex, const std::wstring& prefix) {
  prefixes_.push_back(std::make_pair(prefix, lex));
}

void FtPlugin::AddSuffix(Lex lex, const std::wstring& suffix) {
  suffixes_.push_back(std::make_pair(suffix, lex));
}

bool FtPlugin::MatchAnyof(const std::wstring& str,
                          size_t off,
                          size_t len,
                          Lex* lex) const {
#if JIL_MATCH_WORD_WITH_HASH
  WordLexMap::const_iterator it = anyofs_.find(str.substr(off, len));
  if (it == anyofs_.end()) {
    return false;
  }
  *lex = it->second;
  return true;

#else
  // Binary search the word.
  // Use wcsncmp to avoid a string copy of the word.

  size_t i = 0;
  size_t j = 0;
  size_t k = anyofs_.size();

  while (j < k) {
    i = j + (k - j) / 2;
    const WordLexPair& wlp = anyofs_[i];
    int cmp_result = wcsncmp_(&str[off], wlp.first.c_str(), len);
    if (cmp_result < 0) {
      k = i;
    } else if (cmp_result > 0) {
      j = i + 1;
    } else {
      if (len < wlp.first.size()) {  // E.g., wcsncmp("size", "sizeof", 4)
        k = i;
      } else {
        *lex = wlp.second;
        return true;
      }
    }
  }

  return false;
#endif  // JIL_MATCH_WORD_WITH_HASH
}

size_t FtPlugin::MatchQuote(const std::wstring& str,
                            size_t off,
                            Quote** quote) const {
  for (size_t i = 0; i < quotes_.size(); ++i) {
    size_t matched_off = quotes_[i]->MatchStart(str, off);
    if (matched_off > off) {
      *quote = quotes_[i];
      return matched_off;
    }
  }

  return off;
}

size_t FtPlugin::MatchRegex(const std::wstring& str,
                            size_t off,
                            Lex* lex) const {
  for (size_t i = 0; i < regexs_.size(); ++i) {
    size_t matched_off = regexs_[i]->Match(str, off);
    if (matched_off > off) {
      *lex = regexs_[i]->lex();
      return matched_off;
    }
  }
  return off;
}

bool FtPlugin::MatchPrefix(const std::wstring& str,
                           size_t off,
                           size_t len,
                           Lex* lex) {
  for (size_t i = 0; i < prefixes_.size(); ++i) {
    if (len == prefixes_[i].first.size() &&
        wcsncmp_(&str[off], prefixes_[i].first.c_str(), len) == 0) {
      *lex = prefixes_[i].second;
      return true;
    }
  }
  return false;
}

bool FtPlugin::MatchSuffix(const std::wstring& str,
                           size_t off,
                           size_t len,
                           Lex* lex) {
  for (size_t i = 0; i < suffixes_.size(); ++i) {
    if (len == suffixes_[i].first.size() &&
        wcsncmp_(&str[off], suffixes_[i].first.c_str(), len) == 0) {
      *lex = suffixes_[i].second;
      return true;
    }
  }
  return false;
}

}  // namespace editor
}  // namespace jil
