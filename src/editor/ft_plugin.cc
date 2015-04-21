#include "editor/ft_plugin.h"
#if !JIL_MATCH_WORD_WITH_HASH
#include <algorithm>
#endif
#include "boost/regex.hpp"
#include "editor/indent.h"
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

FtPlugin::IndentFuncMap FtPlugin::indent_funcs_;

FtPlugin::FtPlugin(const FileType& file_type)
    : file_type_(file_type)
    , ignore_case_(false)
    , indent_func_(NULL) {
  wcsncmp_ = wcsncmp;

  InitIndentFunc();
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

  if (regex_quote->lex().major() == kLexComment) {
  }
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

//------------------------------------------------------------------------------

bool FtPlugin::MatchIndentKey(const std::wstring& str,
                              size_t off,
                              size_t len) const {
  const std::vector<std::wstring>& indent_keys = options_.indent_keys;
  for (size_t i = 0; i < indent_keys.size(); ++i) {
    if (indent_keys[i].size() == len) {
      if (wcsncmp_(&str[off], indent_keys[i].c_str(), len) == 0) {
        return true;
      }
    }
  }

  return false;
}

void FtPlugin::InitIndentFunc() {
  if (indent_funcs_.empty()) {
    indent_funcs_[wxT("cfg")] = IndentCfg;
    indent_funcs_[wxT("c")] = IndentCpp;
    indent_funcs_[wxT("cpp")] = IndentCpp;
    indent_funcs_[wxT("csharp")] = IndentCSharp;
    indent_funcs_[wxT("css")] = IndentCss;
    indent_funcs_[wxT("cue")] = IndentCue;
    indent_funcs_[wxT("go")] = IndentGo;
    indent_funcs_[wxT("html")] = IndentHtml;
    indent_funcs_[wxT("java")] = IndentJava;
    indent_funcs_[wxT("javascript")] = IndentJavaScript;
    indent_funcs_[wxT("python")] = IndentPython;
    indent_funcs_[wxT("ruby")] = IndentRuby;
    indent_funcs_[wxT("txt")] = IndentTxt;
    indent_funcs_[wxT("vb")] = IndentVB;
    indent_funcs_[wxT("xml")] = IndentXml;
  }

  IndentFuncMap::iterator it = indent_funcs_.find(file_type_.id);
  if (it != indent_funcs_.end()) {
    indent_func_ = it->second;
  } else {
    indent_func_ = IndentTxt;
  }
}

}  // namespace editor
}  // namespace jil
