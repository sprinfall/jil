#include "editor/relite.h"
#include "editor/defs.h"
#include "editor/util.h"
#include <cassert>

namespace jil {
namespace editor {

namespace relite {

////////////////////////////////////////////////////////////////////////////////

static Repeat RepeatFromChar(wchar_t c) {
  Repeat repeat;

  if (c == L'?') {
    repeat.min = 0;
    repeat.max = 1;
  } else if (c == L'*') {
    repeat.min = 0;
    repeat.max = kNpos;
  } else if (c == L'+') {
    repeat.min = 1;
    repeat.max = kNpos;
  }

  return repeat;
}

////////////////////////////////////////////////////////////////////////////////

Node::~Node() {
}

////////////////////////////////////////////////////////////////////////////////

Atom::Atom(Type type, wchar_t c)
  : type_(type), c_(c) {
}

Atom::~Atom() {
}

// TODO
static bool IsDelimiter(wchar_t c) {
  if (IsSpace(c)) {
    return true;
  }

  if (c == L',' || c == L'/') {
    return true;
  }

  return false;
}

size_t Atom::Match(const std::wstring& str, size_t off) const {
  size_t m = 0;
  size_t i = off;

  for (; m < repeat_.max && i < str.size(); ++m, ++i) {
    if (type_ == kNormal) {
      if (str[i] != c_) {
        break;
      }
    } else if (type_ == kWildcard) {
      // Wildcard matches any charactor.
    } else if (type_ == kSpace) {
      if (!IsSpace(str[i])) {
        break;
      }
    } else if (type_ == kBound) {
      if (!IsDelimiter(str[i])) {
        break;
      }
    } else {
      break;  // Not supported
    }
  }

  if (m >= repeat_.min && m <= repeat_.max) {
    return i;  // NOTE: i might == off
  }

  if (i == str.size()) {
    // Reach the end but still not matched. Check boundary.
    if (type_ == kBound) {
      return i;
    }
  }

  return kNpos;  // Failed to match.
}

////////////////////////////////////////////////////////////////////////////////

Word::Word(const std::wstring& text)
: text_(text) {
}

Word::~Word() {
}

size_t Word::Match(const std::wstring& str, size_t off) const {
  if (SubStringEquals(str, off, text_)) {
    return off + text_.size();
  }
  return kNpos;
}

////////////////////////////////////////////////////////////////////////////////

Regex::Regex(const std::wstring& pattern, int flags)
    : pattern_(pattern), flags_(flags), valid_(false) {
  valid_ = Compile();
}

Regex::~Regex() {
  for (size_t i = 0; i < nodes_.size(); ++i) {
    delete nodes_[i];
  }
  nodes_.clear();
}

size_t Regex::Match(const std::wstring& str, size_t off) const {
  size_t i = off;

  for (size_t n = 0; n < nodes_.size(); ++n) {
    Node* node = nodes_[n];

    // TODO: Refactoring
    // If the node is .? .* or .+, skip to search the match for the next node.
    Atom* atom = dynamic_cast<Atom*>(node);
    if (atom != NULL &&
        atom->type() == Atom::kWildcard &&
        atom->repeat().Variable() &&
        n + 1 < nodes_.size()) {
      const Repeat& repeat = atom->repeat();
      size_t first_i = i + repeat.min;
      size_t last_i = repeat.max == kNpos ? kNpos : i + repeat.max;

      Node* next_node = nodes_[n + 1];
      bool next_matched = false;
      for (size_t k = first_i; k <= last_i && k < str.size(); ++k) {
        size_t match_k = next_node->Match(str, k);
        if (match_k > k && match_k != kNpos) {
          next_matched = true;
          ++n;
          i = match_k;
          break;
        }
      }

      if (!next_matched) {
        return off;
      }

      continue;
    }

    size_t j = node->Match(str, i);
    if (j == kNpos) {
      return off;  // Failed to match
    }

    i = j;
  }

  return i;
}

bool Regex::Compile() {
  bool escaped = false;
  std::wstring word;

  const size_t size = pattern_.size();

  for (size_t i = 0; i < size; ++i) {
    wchar_t c = pattern_[i];

    switch (c) {
    case L'\\':
      if (escaped) {
        escaped = false;
        word.append(1, c);
      } else {
        escaped = true;
      }

      break;

    case L's':
      if (escaped) {  // \s
        escaped = false;
        AddWord(word);
        nodes_.push_back(new Atom(Atom::kSpace));
      } else {
        word.append(1, c);
      }

      break;

    case L'b':
      if (escaped) {  // \b
        escaped = false;
        AddWord(word);
        nodes_.push_back(new Atom(Atom::kBound));
      } else {
        word.append(1, c);
      }

      break;

    case L'.':
      if (escaped) {  // \.
        escaped = false;
        word.append(1, c);
      } else {
        AddWord(word);
        nodes_.push_back(new Atom(Atom::kWildcard));
      }

      break;

    case L'?':
    case L'*':
    case L'+':
      if (escaped) {
        word.append(1, c);
      } else {
        AddWord(word);

        Atom* atom = LastAsAtom();
        if (atom == NULL) {
          return false;
        }

        atom->set_repeat(RepeatFromChar(c));
      }

      break;

    default:
      word.append(1, c);
      break;
    }
  }

  AddWord(word);

  return true;
}

void Regex::AddWord(std::wstring& word) {
  if (word.empty()) {
    return;
  }
  if (word.size() == 1) {
    nodes_.push_back(new Atom(Atom::kNormal, word[0]));
  } else {
    nodes_.push_back(new Word(word));
  }
  word.clear();
}


Atom* Regex::LastAsAtom() {
  if (nodes_.empty()) {
    return NULL;
  }
  // TODO: Avoid dynamic_cast.
  return dynamic_cast<Atom*>(nodes_.back());
}

}  // namespace relite

}  // namespace editor
}  // namespace jil
