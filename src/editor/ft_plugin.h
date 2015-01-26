#ifndef JIL_EDITOR_FT_PLUGIN_H_
#define JIL_EDITOR_FT_PLUGIN_H_
#pragma once

// There's no big performance improvement of using hash map to match ANYOF.
// Test with scanning a C++ file with about 15,000 lines of code:
// - Hash map: 580ms
// - Binary search: 600ms
#define JIL_MATCH_WORD_WITH_HASH 0

#include <map>
#include <utility>
#include <vector>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"

#if JIL_MATCH_WORD_WITH_HASH
#include "boost/unordered_map.hpp"
#endif

#include "wx/string.h"

#include "editor/defs.h"
#include "editor/indent.h"
#include "editor/lex.h"
#include "editor/option.h"

namespace jil {
namespace editor {

// Example: ("true", Lex(kLexConstant, kLexConstantBool))
typedef std::pair<std::wstring, Lex> WordLexPair;

// File type specific options, lex, etc.
class FtPlugin {
public:
  FtPlugin(const FileType& file_type, lua_State* lua_state);
  ~FtPlugin();

  const wxString& id() const {
    return file_type_.id;
  }

  const wxString& name() const {
    return file_type_.name;
  }

  //----------------------------------------------------------------------------
  // Options

  Options& options() {
    return options_;
  }

  //----------------------------------------------------------------------------

  bool IsOperator(wchar_t c) const;
  bool IsDelimiter(wchar_t c) const;

  //----------------------------------------------------------------------------
  // Lex

  bool IsLexAvailable() const {
    return !anyofs_.empty() || !quotes_.empty();
  }

  void SetIgnoreCase(bool ignore_case);

  // \param joined_words Space separated words, e.g., "true false".
  void AddAnyof(Lex lex, const std::wstring& joined_words);

#if !JIL_MATCH_WORD_WITH_HASH
  void SortAnyof();
#endif

  void AddQuote(Quote* quote);
  void AddRegexQuote(RegexQuote* regex_quote);

  // \param pattern Regex pattern, e.g., "^#\s*endif\b".
  void AddRegex(Lex lex, const std::wstring& pattern);

  void AddPrefix(Lex lex, const std::wstring& prefix);
  void AddSuffix(Lex lex, const std::wstring& suffix);

  // Check if str.substr(off, count) is a anyof.
  bool MatchAnyof(const std::wstring& str,
                  size_t off,
                  size_t len,
                  Lex* lex) const;

  // Check if any str.substr(off, x) is a quote start.
  // Return the end index of the quote start or @off if no quote start matched.
  size_t MatchQuote(const std::wstring& str, size_t off, Quote** quote) const;

  size_t MatchRegex(const std::wstring& str, size_t off, Lex* lex) const;

  // Check if str.substr(off, count) is a lex prefix.
  bool MatchPrefix(const std::wstring& str, size_t off, size_t len, Lex* lex);

  bool MatchSuffix(const std::wstring& str, size_t off, size_t len, Lex* lex);

  //----------------------------------------------------------------------------
  // Indent

  luabridge::LuaRef indent_func() const {
    return indent_func_;
  }

  void set_indent_func(const luabridge::LuaRef& indent_func) {
    indent_func_ = indent_func;
  }

  bool MatchIndentKey(const std::wstring& str, size_t off, size_t len) const;

private:
  typedef std::map<wxString, luabridge::LuaRef> IndentFuncMap;
  static IndentFuncMap indent_funcs_;

  FileType file_type_;

  lua_State* lua_state_;

  // File type specific options.
  Options options_;

  // Some file types ignore case for keywords, e.g., VB.
  bool ignore_case_;

  // String compare function.
  // Determined by ignore_case_.
  WcsNCmp wcsncmp_;

#if JIL_MATCH_WORD_WITH_HASH
  typedef boost::unordered_map<std::wstring, Lex> WordLexMap;
  WordLexMap anyofs_;
#else
  std::vector<WordLexPair> anyofs_;
#endif

  std::vector<Quote*> quotes_;
  std::vector<RegexQuote*> regex_quotes_;

  std::vector<Regex*> regexs_;

  // Normally there won't be many prefixes and suffixes. So simply use array
  // and linear search.
  std::vector<WordLexPair> prefixes_;
  std::vector<WordLexPair> suffixes_;

  // Indent function for this file type.
  luabridge::LuaRef indent_func_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_FT_PLUGIN_H_
