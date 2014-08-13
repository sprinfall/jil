#ifndef JIL_EDITOR_KEY_H_
#define JIL_EDITOR_KEY_H_
#pragma once

#include <string>
#include "wx/string.h"

namespace jil {
namespace editor {

// Jil determines key strokes based on key down event instead of char event.
// So the chars ~!@#$%^&*()_+<>?:"{}| in binding config file actually means
// `1234567890-=,./;'[]\ with modifier wxMOD_SHIFT.
extern const std::string kShiftChars;  // "~!@#$%^&*()_+<>?:\"{}|"
extern const std::string kNonShiftChars;  // "`1234567890-=,./;'[]\

// A key consists of a key code and optionally the modifiers. It could also
// has a leader key.
// Examples:
// - Ctrl+J
// - Ctrl+K, Ctrl+K
// - Ctrl+K, U
// - Ctrl+Shift+K
class Key {
 public:
  Key() : data_(0) {
  }

  Key(int code, int modifiers = wxMOD_NONE) {
    Set(code, modifiers);
  }

  Key(int leader_code,
      int leader_modifiers,
      int code,
      int modifiers = wxMOD_NONE) {
    Set(leader_code, leader_modifiers, code, modifiers);
  }

  int data() const { return data_; }

  int code() const { return data_ & 0x03ff; }
  int modifiers() const { return (data_ & 0xfc00) >> 10; }
  int leader_code() const { return (data_ & 0x03ff0000) >> 16; }
  int leader_modifiers() const { return (data_ & 0xfc000000) >> 26; }

  Key leader() const {
    return Key(leader_code(), leader_modifiers());
  }
  void set_leader(int leader_code, int leader_modifiers) {
    data_ = (data_ & 0xffff) | (Make(leader_code, leader_modifiers) << 16);
  }
  void set_leader(Key leader_key) {
    set_leader(leader_key.code(), leader_key.modifiers());
  }

  void Set(int code, int modifiers = wxMOD_NONE) {
    data_ = Make(code, modifiers);
  }

  void Set(int leader_code,
           int leader_modifiers,
           int code,
           int modifiers = wxMOD_NONE) {
    data_ = Make(code, modifiers) |
            (Make(leader_code, leader_modifiers) << 16);
  }

  void Reset() { data_ = 0; }

  bool IsEmpty() const { return data_ == 0; }

  wxString ToString() const;

 private:
  int Make(int code, int modifiers) {
    return (code | (modifiers << 10));
  }

 private:
  // Encode key stroke in the last 4 bytes of int.

  //         /    leader    \
  // 32bits: ****** ********** ****** **********
  //          mod      code     mod      code
  int data_;
};

inline bool operator==(Key lhs, Key rhs) {
  return lhs.data() == rhs.data();
}
inline bool operator!=(Key lhs, Key rhs) {
  return lhs.data() != rhs.data();
}
inline bool operator<(Key lhs, Key rhs) {
  return lhs.data() < rhs.data();
}
inline bool operator>(Key lhs, Key rhs) {
  return lhs.data() > rhs.data();
}

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_KEY_H_
