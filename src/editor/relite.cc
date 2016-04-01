#include "editor/relite.h"
#include <cassert>
#include <cwctype>
#include "editor/defs.h"
#include "editor/util.h"

namespace jil {
namespace editor {

namespace relite {

////////////////////////////////////////////////////////////////////////////////

static inline bool WcharIcCmp(wchar_t c1, wchar_t c2) {
  return std::towlower(c1) == std::towlower(c2);
}

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

// Sorted word delimiters.
static const std::wstring kDelimiters = L"!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";

// NOTE:
// According to test, std::wstring.find() is much faster than std::binary_search.
static bool IsDelimiter(wchar_t c) {
  if (IsSpace(c)) {
    return true;
  }
  return kDelimiters.find(c) != std::wstring::npos;
}

////////////////////////////////////////////////////////////////////////////////

size_t Atom::Match(const std::wstring& str, size_t off, bool ignore_case) const {
  size_t m = 0;
  size_t i = off;

  for (; m < repeat_.max && i < str.size(); ++m) {
    if (!MatchChar(str[i], ignore_case)) {
      break;
    }
    if (type_ != kBound) {  // Don't take bound into account!
      ++i;
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

  return kNpos;
}

bool Atom::Determinable() const {
  if (type_ == kWildcard && repeat_.Variable()) {
    return false;
  }
  return true;
}

bool Atom::MatchChar(wchar_t c, bool ignore_case) const {
  switch (type_) {
    case kNormal:
      if (!ignore_case) {
        if (c != c_) {
          return false;
        }
      } else {
        if (!WcharIcCmp(c, c_)) {
          return false;
        }
      }
      break;

    case kWildcard:
      // Wildcard matches any charactor.
      break;

    case kSpace:
      if (!IsSpace(c)) {
        return false;
      }
      break;

    case kDigit:
      if (!std::iswdigit(c)) {
        return false;
      }
      break;

    case kBound:
      if (!IsDelimiter(c)) {
        return false;
      }
      break;

    default:
      return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

size_t Word::Match(const std::wstring& str, size_t off, bool ignore_case) const {
  WcsNCmp cmp = ignore_case ? wcsnicmp : wcsncmp;
  if (SubStringEquals(str, off, text_, cmp)) {
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

size_t Regex::Match(const std::wstring& str, size_t off, Sub* subs, size_t subs_count) const {
  bool ignore_case = (flags_ & kIgnoreCase) != 0;

  size_t i = off;
  size_t match_i = kNpos;

  bool grouped = false;
  size_t sub_index = 0;

  for (size_t n = 0; n < nodes_.size(); ++n) {
    Node* node = nodes_[n];

    Group* group = dynamic_cast<Group*>(node);
    if (group != NULL) {
      if (sub_index < subs_count) {
        if (grouped) {
          subs[sub_index].len = i - subs[sub_index].off;
          grouped = false;
          ++sub_index;
        } else {
          subs[sub_index].off = i;
          grouped = true;
        }
      }
      continue;
    }

    // If the atom is undeterminable, match the next node instead.
    Atom* atom = node->AsAtom();
    if (atom != NULL && !atom->Determinable() && n + 1 < nodes_.size()) {
      // Get the index range for matching the next node.
      const Repeat& repeat = atom->repeat();
      size_t from = i + repeat.min;
      size_t to = repeat.max == kNpos ? kNpos : i + repeat.max;

      match_i = Match(str, from, to, nodes_[n + 1], ignore_case);
      if (match_i == kNpos) {
        return off;
      } else {
        ++n;
        i = match_i;
        continue;
      }
    }

    if (atom != NULL && atom->type() == Atom::kEnd) {  // $
      if (i != str.size()) {
        return off;
      } else {
        return i;
      }
    }

    match_i = node->Match(str, i, ignore_case);

    if (match_i == kNpos) {
      return off;
    } else {
      i = match_i;
    }
  }

  return i;
}

size_t Regex::Match(const std::wstring& str,
                    size_t from,
                    size_t to,
                    const Node* node,
                    bool ignore_case) const {
  for (size_t i = from; i <= to && i < str.size(); ++i) {
    size_t match_i = node->Match(str, i, ignore_case);
    if (match_i > i && match_i != kNpos) {
      return match_i;
    }
  }
  return kNpos;
}

bool Regex::Compile() {
  bool escaped = false;
  bool grouped = false;
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
          CompileWord(word);
          nodes_.push_back(new Atom(Atom::kSpace));
        } else {
          word.append(1, c);
        }

        break;

      case L'd':
        if (escaped) {  // \d
          escaped = false;
          CompileWord(word);
          nodes_.push_back(new Atom(Atom::kDigit));
        } else {
          word.append(1, c);
        }

        break;

      case L'b':
        if (escaped) {  // \b
          escaped = false;
          CompileWord(word);
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
          CompileWord(word);
          nodes_.push_back(new Atom(Atom::kWildcard));
        }

        break;

      case L'?':
      case L'*':
      case L'+':
        if (escaped) {
          escaped = false;
          word.append(1, c);
        } else {
          CompileWord(word);

          Atom* atom = nodes_.empty() ? NULL : nodes_.back()->AsAtom();
          if (atom != NULL) {
            atom->set_repeat(RepeatFromChar(c));
          } else {
            return false;
          }
        }

        break;

      case L'(':
        if (escaped) {
          escaped = false;
          word.append(1, c);
        } else {
          if (grouped) {
            return false;
          }
          CompileWord(word);
          nodes_.push_back(new Group);
          grouped = true;  // Open a new group.
        }
        break;

      case L')':
        if (escaped) {
          escaped = false;
          word.append(1, c);
        } else {
          if (!grouped) {
            return false;
          }
          CompileWord(word);
          nodes_.push_back(new Group);
          grouped = false;  // Close current group.
        }
        break;

      case L'[':
      case L']':
        if (escaped) {
          escaped = false;
          word.append(1, c);
        } else {
          return false;  // Not supported yet
        }
        break;

      case L'$':
        if (escaped) {
          escaped = false;
          word.append(1, c);
        } else {
          if (i != size - 1) {
            // $ should always be the last char.
            return false;
          }
          CompileWord(word);
          nodes_.push_back(new Atom(Atom::kEnd));
        }

        break;

      default:
        word.append(1, c);
        break;
    }
  }

  if (grouped) {  // Unclosed group.
    return false;
  }

  CompileWord(word);

  return true;
}

void Regex::CompileWord(std::wstring& word) {
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

}  // namespace relite

}  // namespace editor
}  // namespace jil
