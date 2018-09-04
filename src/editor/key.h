#ifndef EDITOR_KEY_H_
#define EDITOR_KEY_H_

#include <string>

#include "wx/string.h"

#include "editor/compile_config.h"

namespace editor {

// Jil determines key strokes based on key down event instead of char event.
// So the chars ~!@#$%^&*()_+<>?:"{}| in binding config file actually means
// `1234567890-=,./;'[]\ with modifier wxMOD_SHIFT.
extern const std::string kShiftChars;  // "~!@#$%^&*()_+<>?:\"{}|"
extern const std::string kNonShiftChars;  // "`1234567890-=,./;'[]\

// A key consists of a key code and optionally the modifiers.
// Examples:
// - Ctrl+J
// - Ctrl+Shift+K
// It could also has a leader key if it's enabled.
// Examples:
// - Ctrl+K, Ctrl+K
// - Ctrl+K, U

class Key {
public:
  Key() : data_(0) {
  }

  explicit Key(int code, int modifiers = wxMOD_NONE) {
    Set(code, modifiers);
  }

#if JIL_ENABLE_LEADER_KEY
  Key(int leader_code, int leader_modifiers, int code,
      int modifiers = wxMOD_NONE) {
    Set(leader_code, leader_modifiers, code, modifiers);
  }
#endif  // JIL_ENABLE_LEADER_KEY

  int data() const { return data_; }

  int code() const { return data_ & 0x03ff; }
  int modifiers() const { return (data_ & 0xfc00) >> 10; }

#if JIL_ENABLE_LEADER_KEY
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
#endif  // JIL_ENABLE_LEADER_KEY

  void Set(int code, int modifiers = wxMOD_NONE) {
    data_ = Make(code, modifiers);
  }

#if JIL_ENABLE_LEADER_KEY
  void Set(int leader_code, int leader_modifiers, int code,
           int modifiers = wxMOD_NONE) {
    data_ = Make(code, modifiers) | (Make(leader_code, leader_modifiers) << 16);
  }
#endif  // JIL_ENABLE_LEADER_KEY

  void Reset() { data_ = 0; }

  bool IsEmpty() const { return data_ == 0; }

  // Convert modifiers to accelerator flags.
  int GetAccelFlags() const;

  wxString ToString() const;

private:
  int Make(int code, int modifiers) {
    return (code | (modifiers << 10));
  }

private:
  // Encode key stroke in the last 4 bytes of int.
  //            Optionally (When leader key is enabled)
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

#endif  // EDITOR_KEY_H_
