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

class Node {
public:
  virtual ~Node() = 0;

  // Return kNpos(-1) if failed to match.
  virtual size_t Match(const std::wstring& str, size_t off) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class Atom : public Node {
public:
  enum Type {
    kNormal = 0,  // Normal charactors: 'a', 'b', 'c', etc.
    kWildcard,    // .
    kSpace,       // \s
    kDigit,       // \d
    kBound,       // \b
    // ... Not supported yet!
  };

public:
  Atom(Type type, wchar_t c = 0);
  virtual ~Atom();

  virtual size_t Match(const std::wstring& str, size_t off) const override;

  Type type() const {
    return type_;
  }

  const Repeat& repeat() const {
    return repeat_;
  }
  void set_repeat(const Repeat& repeat) {
    repeat_ = repeat;
  }

private:
  Type type_;
  wchar_t c_;
  Repeat repeat_;
};

////////////////////////////////////////////////////////////////////////////////

class Word : public Node {
public:
  explicit Word(const std::wstring& text);
  virtual ~Word();

  virtual size_t Match(const std::wstring& str, size_t off) const override;

private:
  std::wstring text_;
};

////////////////////////////////////////////////////////////////////////////////

class Regex {
public:
  enum Flag {
    kIgnoreCase = 1,
  };

public:
  Regex(const std::wstring& pattern, int flags);
  ~Regex();

  // Return true if the pattern is valid (i.e., successfully compiled).
  bool valid() const {
    return valid_;
  }

  size_t Match(const std::wstring& str, size_t off) const;

private:
  bool Compile();

  // TODO: Rename
  void AddWord(std::wstring& word);

  // Get last node as Atom.
  // Return NULL is the last node is not an Atom.
  Atom* LastAsAtom();

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
