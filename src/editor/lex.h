#ifndef EDITOR_LEX_H_
#define EDITOR_LEX_H_

#include "editor/compile_config.h"

#if !JIL_LEX_USE_RELITE
#include <regex>
#endif

#include <string>
#include <vector>

#include "editor/defs.h"

namespace editor {

#if JIL_LEX_USE_RELITE
namespace relite {
class Regex;
}
#endif  // JIL_LEX_USE_RELITE

////////////////////////////////////////////////////////////////////////////////

// Lex (major and minor) types.

enum LexMajorType {
  kLexNone = 0,
  kLexComment,
  kLexConstant,
  kLexIdentifier,
  kLexStatement,
  kLexPackage,
  kLexPreProc,
  kLexType,
  kLexUrl,
  kLexError,

  kLexMajorTypeCount
};

enum LexConstant {
  kLexConstantOther = 0,
  kLexConstantChar,       // 'c', '\n'
  kLexConstantString,     // "string"
  kLexConstantNumber,     // 123, 0xff, 3.14, 1.23e10
  kLexConstantBool,       // true, false
  kLexConstantNull,       // NULL, nil, Null, etc.

  kLexConstantCount
};

enum LexIdentifier {
  kLexIdentifierOther = 0,

  kLexIdentifierVariable,

  // For Python, given
  //   def f(): ...
  // then f is a function identifier.
  kLexIdentifierFunction,

  // For C++, given
  //   class A { }
  // then A is a class identifier.
  kLexIdentifierClass,
};

enum LexStatement {
  kLexStatementOther = 0,
  kLexStatementConditional,   // if, then, else, endif, switch, etc.
  kLexStatementRepeat,        // for, do, while, etc.
  kLexStatementOperator,      // sizeof, new, delete, etc.
  kLexStatementException,     // try, catch, throw

  kLexStatementCount
};

enum LexType {
  kLexTypeOther = 0,
  kLexTypeQualifier,    // static, auto, register, const, etc.
  kLexTypeStruct,       // class, struct, union, etc.

  kLexTypeCount
};

////////////////////////////////////////////////////////////////////////////////

class Lex {
public:
  Lex() : data_(0) {
  }

  explicit Lex(int major, int minor = 0) {
    Set(major, minor);
  }

  int data() const {
    return data_;
  }

  int major() const {
    return (data_ & 0xffff0000) >> 16;
  }

  int minor() const {
    return data_ & 0xffff;
  }

  void Set(int major, int minor = 0) {
    data_ = Make(major, minor);
  }

  void Reset() {
    data_ = 0;
  }

  bool IsEmpty() const {
    return data_ == 0;
  }

private:
  int Make(int major, int minor) {
    return (minor | (major << 16));
  }

private:
  int data_;
};

inline bool operator==(Lex lhs, Lex rhs) {
  return lhs.data() == rhs.data();
}
inline bool operator!=(Lex lhs, Lex rhs) {
  return lhs.data() != rhs.data();
}
inline bool operator<(Lex lhs, Lex rhs) {
  return lhs.data() < rhs.data();
}
inline bool operator>(Lex lhs, Lex rhs) {
  return lhs.data() > rhs.data();
}

////////////////////////////////////////////////////////////////////////////////

class LexElem {
public:
  LexElem() : off(0), len(0) {
  }

  LexElem(Coord _off, Coord _len, Lex _lex)
      : off(_off), len(_len), lex(_lex) {
  }

  void Set(Coord _off, Coord _len, Lex _lex) {
    off = _off;
    len = _len;
    lex = _lex;
  }

  void Reset() {
    off = 0;
    len = 0;
    lex.Reset();
  }

  Coord off;
  Coord len;
  Lex lex;
};

////////////////////////////////////////////////////////////////////////////////

enum QuotePart {
  kQuoteStart,
  kQuoteBody,
  kQuoteEnd
};

enum QuoteFlag {
  // Multi-line quote.
  // E.g., C/C++ comment: /* */
  kQuoteMultiLine = 1,

  // EOL can be escaped. (Usually for single-line quote)
  // E.g., C/C++ string:
  //   "first line string \
  // second line string"
  kQuoteEscapeEol = 2,
};

class Quote {
public:
  Quote(Lex lex,
        const std::wstring& start,
        const std::wstring& end,
        int flags);

  virtual ~Quote();

  Lex lex() const {
    return lex_;
  }

  const std::wstring& start() const {
    return start_;
  }

  const std::wstring& end() const {
    return end_;
  }

  int flags() const {
    return flags_;
  }

  void set_name(const std::string& name) {
    name_ = name;
  }
  const std::string& name() const {
    return name_;
  }

  bool multi_line() const {
    return (flags_ & kQuoteMultiLine) != 0;
  }

  bool escape_eol() const {
    return (flags_ & kQuoteEscapeEol) != 0;
  }

  void set_ignore_case(bool ignore_case) {
    ignore_case_ = ignore_case;
  }

  std::size_t MatchStart(const std::wstring& str, std::size_t off) const;

protected:
  Lex lex_;

  std::wstring start_;
  std::wstring end_;
  int flags_;

  std::string name_;  // Optional.

  // Match with case ignored. E.g., VB's comment: REM
  bool ignore_case_;
};

class QuoteElem {
public:
  Quote* quote;
  Coord off;
  Coord len;
  QuotePart part;
};

////////////////////////////////////////////////////////////////////////////////

// NOTE:
// The end of regex quote only supports back reference "\1".
class RegexQuote : public Quote {
public:
  RegexQuote(Lex lex,
             const std::wstring& start,
             const std::wstring& end,
             int flags);
  virtual ~RegexQuote();

  std::size_t MatchStart(const std::wstring& str, std::size_t off,
                         std::wstring* concrete_end) const;

  // Add a concrete quote.
  void AddQuote(Quote* quote) const {
    quotes_.push_back(quote);
  }

private:
  void CreateRegex();

private:
#if JIL_LEX_USE_RELITE
  relite::Regex*  start_re_;
#else
  std::wregex*    start_re_;
#endif  // JIL_LEX_USE_RELITE

  // Concrete quotes created from this regex quote.
  mutable std::vector<Quote*> quotes_;
};

////////////////////////////////////////////////////////////////////////////////

class Regex {
public:
  Regex(const std::wstring& pattern, bool ignore_case);
  virtual ~Regex();

  void set_lex(Lex lex) {
    lex_ = lex;
  }
  void set_lex(LexMajorType major, int minor = 0) {
    lex_.Set(major, minor);
  }

  Lex lex() const {
    return lex_;
  }

  std::size_t Match(const std::wstring& str, std::size_t off) const;

private:
  Lex lex_;
  std::wstring pattern_;

#if JIL_LEX_USE_RELITE
  relite::Regex*  re_;
#else
  std::wregex*    re_;
#endif  // JIL_LEX_USE_RELITE
};

////////////////////////////////////////////////////////////////////////////////

class LexComment {
public:
  bool IsEmpty() const {
    return start.empty();
  }

  std::wstring start;
  std::wstring end;  // Optional; empty for single line comment.
};

}  // namespace editor

#endif  // EDITOR_LEX_H_
