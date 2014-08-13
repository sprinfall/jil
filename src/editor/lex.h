#ifndef JIL_EDITOR_LEX_H_
#define JIL_EDITOR_LEX_H_
#pragma once

#include <string>
#include "boost/regex_fwd.hpp"
#include "editor/compile_config.h"
#include "editor/defs.h"

namespace jil {
namespace editor {

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
  kLexSpecial,  // http URL, etc.

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
  kLexIdentifierFunction,
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
  kLexTypeStruct,       // struct, union, class, etc.

  kLexTypeCount
};

////////////////////////////////////////////////////////////////////////////////

class Lex {
 public:
  Lex() : data_(0) {
  }

  Lex(int major, int minor = 0) {
    Set(major, minor);
  }

  int data() const { return data_; }

  int major() const { return (data_ & 0xffff0000) >> 16; }
  int minor() const { return data_ & 0xffff; }

  void Set(int major, int minor = 0) {
    data_ = Make(major, minor);
  }

  void Reset() { data_ = 0; }

  bool IsEmpty() const { return data_ == 0; }

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

struct LexElement {
  LexElement()
      : off(0), len(0) {
  }

  LexElement(Coord _off, Coord _len, Lex _lex)
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

class Quote {
 public:
  enum Part {
    kStart,
    kBody,
    kEnd
  };

  enum Flag {
    // Multi-line quote.
    // E.g., C/C++ comment: /* */
    kMultiLine = 1,

    // EOL can be escaped. (Usually for single-line quote)
    // E.g., C/C++ string:
    //   "first line string \
    // second line string"
    kEscapeEol = 2,
  };

 public:
  Quote();
  Quote(Lex lex,
        const std::wstring& start,
        const std::wstring& end,
        int flags);

  virtual ~Quote();

  void Set(Lex lex,
           const std::wstring& start,
           const std::wstring& end,
           int flags);

  Lex lex() const { return lex_; }

  const std::wstring& start() const { return start_; }
  const std::wstring& end() const { return end_; }

  bool HasEnd() const { return !end_.empty(); }
  bool NoEnd() const { return end_.empty(); }

  bool multi_line() const { return (flags_ & kMultiLine) != 0; }
  bool escape_eol() const { return (flags_ & kEscapeEol) != 0; }

  void set_ignore_case(bool ignore_case) {
    ignore_case_ = ignore_case;
  }

  virtual size_t MatchStart(const std::wstring& str, size_t off) const;

 protected:
  Lex lex_;
  std::wstring start_;
  std::wstring end_;
  int flags_;
  // Match with case ignored.
  // E.g., VB's comment: REM
  bool ignore_case_;
};

#if JIL_LEX_REGEX_QUOTE_START

////////////////////////////////////////////////////////////////////////////////

class RegexQuote : public Quote {
 public:
  RegexQuote();

  virtual ~RegexQuote();

  virtual size_t MatchStart(const std::wstring& str, size_t off) const override;

 private:
  boost::wregex* start_re_;
};

#endif  // JIL_LEX_REGEX_QUOTE_START

////////////////////////////////////////////////////////////////////////////////

class Regex {
 public:
  Regex();
  virtual ~Regex();

  void set_lex(Lex lex) { lex_ = lex; }
  void set_lex(LexMajorType major, int minor = 0) { lex_.Set(major, minor); }

  Lex lex() const { return lex_; }

  // Put ignore_case here because regex_constants::icase
  void set_pattern(const std::wstring& pattern, bool ignore_case);

  size_t Match(const std::wstring& str, size_t off) const;

 private:
  Lex lex_;
  std::wstring pattern_;
  boost::wregex* re_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_LEX_H_
