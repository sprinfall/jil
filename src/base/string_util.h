#ifndef BASE_STRING_UTIL_H_
#define BASE_STRING_UTIL_H_
#pragma once

// char, wchar_t, std::string and std::wstring related utilities.
// Please don't put wxString related utilities here.

#include "boost/lexical_cast.hpp"

namespace base {

// Usage:
//   LexicalCast<int>("123", 0);
//   LexicalCast<std::string>(123, "");
template <typename To, typename From>
To LexicalCast(const From& input, const To& default_output) {
  try {
    return boost::lexical_cast<To>(input);
  } catch (boost::bad_lexical_cast&) {
    return default_output;
  }
}

// Usage:
//   LexicalCast<int>("123");
//   LexicalCast<std::string>(123);
template <typename To, typename From>
To LexicalCast(const From& input) {
  try {
    return boost::lexical_cast<To>(input);
  } catch (boost::bad_lexical_cast&) {
    return To();
  }
}

}  // namespace base

#endif  // BASE_STRING_UTIL_H_
