#include "editor/ft_plugin.h"
#if !JIL_MATCH_WORD_WITH_HASH
#include <algorithm>
#endif
#include "editor/lex.h"

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

FtPlugin::FtPlugin(const FileType& file_type, lua_State* lua_state)
    : file_type_(file_type)
    , lua_state_(lua_state)
    , ignore_case_(false)
    , indent_func_(lua_state_) {
  wcsncmp_ = wcsncmp;
}

FtPlugin::~FtPlugin() {
  ClearContainer(&quotes_);
  ClearContainer(&regexs_);
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
        anyofs_.push_back(std::make_pair(joined_words.substr(p, i - p), lex));
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
#if JIL_USE_CPP11
  anyofs_.shrink_to_fit();  // Shrink to save memory.
#endif  // JIL_USE_CPP11

  std::sort(anyofs_.begin(), anyofs_.end());
}

#endif  // !JIL_MATCH_WORD_WITH_HASH

void FtPlugin::AddQuote(Quote* quote) {
  assert(!quote->start().empty());

  quote->set_ignore_case(ignore_case_);
  quotes_.push_back(quote);

  if (quote->lex().major() == kLexComment) {
    if (quote->end().empty()) {
      if (sline_comment_.IsEmpty()) {
        sline_comment_.start = quote->start();
      }
    } else {
      if (block_comment_.IsEmpty()) {
        block_comment_.start = quote->start();
        block_comment_.end = quote->end();
      }
    }
  }
}

void FtPlugin::AddRegexQuote(RegexQuote* regex_quote) {
  regex_quote->set_ignore_case(ignore_case_);
  regex_quotes_.push_back(regex_quote);
}

void FtPlugin::AddRegex(Lex lex, const std::wstring& pattern) {
  Regex* regex = new Regex(pattern, ignore_case_);
  regex->set_lex(lex);
  regexs_.push_back(regex);
}

void FtPlugin::AddPrefix(Lex lex, const std::wstring& prefix) {
  prefixes_.push_back(std::make_pair(prefix, lex));
}

void FtPlugin::AddRegexPrefix(Lex lex, const std::wstring& pattern) {
  Regex* regex = new Regex(pattern, ignore_case_);
  regex->set_lex(lex);
  regex_prefixes_.push_back(regex);
}

void FtPlugin::AddPrev(Lex lex, const std::wstring& prev) {
  prevs_.push_back(std::make_pair(prev, lex));
}

void FtPlugin::AddNext(Lex lex, const std::wstring& next) {
  nexts_.push_back(std::make_pair(next, lex));
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

size_t FtPlugin::MatchQuote(const std::wstring& str, size_t off, Quote** quote) const {
  // Firstly, match regex quotes.
  if (!regex_quotes_.empty()) {
    std::wstring concrete_end;

    for (size_t i = 0; i < regex_quotes_.size(); ++i) {
      RegexQuote* regex_quote = regex_quotes_[i];
      size_t matched_off = regex_quote->MatchStart(str, off, &concrete_end);

      if (matched_off > off) {
        // Create a concrete quote from this regex quote.
        std::wstring concrete_start = str.substr(off, matched_off - off);

        Quote* concrete_quote = new Quote(regex_quote->lex(),
                                          concrete_start,
                                          concrete_end,
                                          regex_quote->flags());

        // Keep the concrete quote in the regex quote.
        regex_quote->AddQuote(concrete_quote);

        *quote = concrete_quote;
        return matched_off;
      }
    }
  }

  for (size_t i = 0; i < quotes_.size(); ++i) {
    size_t matched_off = quotes_[i]->MatchStart(str, off);
    if (matched_off > off) {
      *quote = quotes_[i];
      return matched_off;
    }
  }

  return off;
}

size_t FtPlugin::MatchRegex(const std::wstring& str, size_t off, Lex* lex) const {
  for (size_t i = 0; i < regexs_.size(); ++i) {
    size_t matched_off = regexs_[i]->Match(str, off);
    if (matched_off > off) {
      *lex = regexs_[i]->lex();
      return matched_off;
    }
  }
  return off;
}

bool FtPlugin::MatchPrefix(const std::wstring& str, size_t off, size_t len, Lex* lex) const {
  for (size_t i = 0; i < prefixes_.size(); ++i) {
    const std::wstring& word = prefixes_[i].first;
    if (len >= word.size() && wcsncmp_(&str[off], word.c_str(), word.size()) == 0) {
      *lex = prefixes_[i].second;
      return true;
    }
  }

  for (size_t i = 0; i < regex_prefixes_.size(); ++i) {
    size_t matched_off = regex_prefixes_[i]->Match(str, off);
    if (matched_off > off && matched_off <= off + len) {
      *lex = regex_prefixes_[i]->lex();
      return true;
    }
  }

  return false;
}

bool FtPlugin::MatchPrev(const std::wstring& str, size_t off, size_t len, Lex* lex) const {
  for (size_t i = 0; i < prevs_.size(); ++i) {
    if (len == prevs_[i].first.size() &&
        wcsncmp_(&str[off], prevs_[i].first.c_str(), len) == 0) {
      *lex = prevs_[i].second;
      return true;
    }
  }
  return false;
}

bool FtPlugin::MatchNext(const std::wstring& str, size_t off, size_t len, Lex* lex) const {
  for (size_t i = 0; i < nexts_.size(); ++i) {
    if (len == nexts_[i].first.size() &&
        wcsncmp_(&str[off], nexts_[i].first.c_str(), len) == 0) {
      *lex = nexts_[i].second;
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------

// TODO: If it's a new error, display in status bar.
void FtPlugin::AddLuaIndentError(int ln, const std::string& msg) {
  std::map<int, std::string>::const_iterator it = lua_indent_errors_.find(ln);
  if (it == lua_indent_errors_.end()) {
    lua_indent_errors_[ln] = msg;
  }
}

bool FtPlugin::MatchIndentKey(const std::wstring& str,
                              size_t off,
                              size_t len) const {
  const std::vector<std::wstring>& indent_keys = options_.text.indent_keys;
  for (size_t i = 0; i < indent_keys.size(); ++i) {
    if (indent_keys[i].size() == len) {
      if (wcsncmp_(&str[off], indent_keys[i].c_str(), len) == 0) {
        return true;
      }
    }
  }

  return false;
}

}  // namespace editor
}  // namespace jil
