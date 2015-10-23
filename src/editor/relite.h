#ifndef JIL_EDITOR_RELITE_H_
#define JIL_EDITOR_RELITE_H_
#pragma once

// Super fast regex only used for lex scanning.

#include <string>
#include <vector>

namespace jil {
namespace editor {

namespace relite {

////////////////////////////////////////////////////////////////////////////////

enum Flag {
  kIgnoreCase = 1,
};

class Sub {
public:
  size_t off;
  size_t len;
};

class Repeat {
public:
  Repeat() : min(1), max(1) {
  }

  bool Variable() const {
    return max > min;
  }

  size_t min;
  size_t max;
};

////////////////////////////////////////////////////////////////////////////////

class Atom;

class Node {
public:
  virtual ~Node() {
  }

  virtual Atom* AsAtom() {
    return NULL;
  }

  // Return kNpos if failed to match.
  virtual size_t Match(const std::wstring& str,
                       size_t off,
                       bool ignore_case) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class Atom : public Node {
public:
  enum Type {
    kNormal = 0,  // a, b, c, etc.
    kWildcard,    // .
    kSpace,       // \s
    kDigit,       // \d
    kBound,       // \b
  };

public:
  Atom(Type type, wchar_t c = 0) : type_(type), c_(c) {
  }

  virtual ~Atom() {
  }

  virtual Atom* AsAtom() override  {
    return this;
  }

  virtual size_t Match(const std::wstring& str,
                       size_t off,
                       bool ignore_case) const override;

  Type type() const {
    return type_;
  }

  const Repeat& repeat() const {
    return repeat_;
  }
  void set_repeat(const Repeat& repeat) {
    repeat_ = repeat;
  }

  // Currently, only variable wildcards (.? .* or .+) are undeterminable.
  bool Determinable() const;

private:
  Type type_;
  wchar_t c_;
  Repeat repeat_;
};

////////////////////////////////////////////////////////////////////////////////

class Word : public Node {
public:
  explicit Word(const std::wstring& text) : text_(text) {
  }

  virtual ~Word() {
  }

  virtual size_t Match(const std::wstring& str,
                       size_t off,
                       bool ignore_case) const override;

private:
  std::wstring text_;
};

////////////////////////////////////////////////////////////////////////////////

class Group : public Node {
public:
  virtual size_t Match(const std::wstring& str,
                       size_t off,
                       bool ignore_case) const override {
    return off;  // Group doesn't match anything.
  }
};

////////////////////////////////////////////////////////////////////////////////

class Regex {
public:
  Regex(const std::wstring& pattern, int flags);
  ~Regex();

  // Return true if the pattern is valid (i.e., successfully compiled).
  bool valid() const {
    return valid_;
  }

  size_t Match(const std::wstring& str,
               size_t off,
               Sub* subs = NULL,
               size_t subs_count = 0) const;

private:
  bool Compile();
  void CompileWord(std::wstring& word);

  // Match the node in the given range.
  size_t Match(const std::wstring& str,
               size_t from,
               size_t to,
               const Node* node,
               bool ignore_case) const;

private:
  std::wstring pattern_;
  int flags_;

  // Compiled result.
  std::vector<Node*> nodes_;

  // If the pattern is valid or not.
  bool valid_;
};

}  // namespace relite

}  // namespace editor
}  // namespace jil


#endif  // JIL_EDITOR_RELITE_H_
