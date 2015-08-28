#include "editor/key.h"
#include "wx/accel.h"

namespace jil {
namespace editor {

const std::string kShiftChars = "~!@#$%^&*()_+<>?:\"{}|";
const std::string kNonShiftChars = "`1234567890-=,./;'[]\\";

wxString KeyCodeName(int key_code) {
  if (key_code >= 33 && key_code <= 126) {
    // Standard ASCII characters.
    return wxChar(key_code);
  } else {
    switch (key_code) {
    case WXK_BACK:
      return wxT("Back");
    case WXK_TAB:
      return wxT("Tab");
    case WXK_RETURN:
      return wxT("Enter");
    case WXK_ESCAPE:
      return wxT("ESC");
    case WXK_SPACE:
      return wxT("Space");
    case WXK_DELETE:
      return wxT("Del");
    case WXK_END:
      return wxT("End");
    case WXK_HOME:
      return wxT("Home");
    case WXK_LEFT:
      return wxT("Left");
    case WXK_RIGHT:
      return wxT("Right");
    case WXK_UP:
      return wxT("Up");
    case WXK_DOWN:
      return wxT("Down");
    case WXK_INSERT:
      return wxT("Ins");
    case WXK_F1:
      return wxT("F1");
    case WXK_F2:
      return wxT("F2");
    case WXK_F3:
      return wxT("F3");
    case WXK_F4:
      return wxT("F4");
    case WXK_F5:
      return wxT("F5");
    case WXK_F6:
      return wxT("F6");
    case WXK_F7:
      return wxT("F7");
    case WXK_F8:
      return wxT("F8");
    case WXK_F9:
      return wxT("F9");
    case WXK_F10:
      return wxT("F10");
    case WXK_F11:
      return wxT("F11");
    case WXK_F12:
      return wxT("F12");
    case WXK_PAGEDOWN:
      return wxT("PageDown");
    case WXK_PAGEUP:
      return wxT("PageUp");
    }

    return wxEmptyString;
  }
}

static wxString& AddKeyPart(wxString& str, const wxString& part, const wxString& sep = wxT("+")) {
  if (!str.empty()) {
    str += sep;
  }
  str += part;
  return str;
}

wxString KeyModifiersName(int modifiers) {
  if (modifiers == 0) {
    return wxEmptyString;
  }

  wxString name;

  // NOTE: Don't support wxMOD_WIN.

#if defined(__WXMAC__)
  if ((modifiers & wxMOD_RAW_CONTROL) != 0) {
    AddKeyPart(name, wxT("Ctrl"));  // TODO
  }

  if ((modifiers & wxMOD_CMD) != 0) {
    AddKeyPart(name, wxT("Cmd"));  // TODO
  }
#else
  if ((modifiers & wxMOD_CONTROL) != 0) {
    AddKeyPart(name, wxT("Ctrl"));
  }
#endif  // #if defined(__WXMAC__)

  if ((modifiers & wxMOD_ALT) != 0) {
    AddKeyPart(name, wxT("Alt"));
  }

  if ((modifiers & wxMOD_SHIFT) != 0) {
    AddKeyPart(name, wxT("Shift"));
  }

  return name;
}

static wxString SingleKeyName(int key_data) {
  int modifiers = (key_data & 0xfc00) >> 10;
  int code = key_data & 0x03ff;

  // Example: Shift+4 -> $
  if ((modifiers & wxMOD_SHIFT) != 0) {
    size_t pos = kNonShiftChars.find(static_cast<char>(code));
    if (pos != std::string::npos) {
      code = kShiftChars[pos];
      modifiers = modifiers & ~wxMOD_SHIFT;
    }
  }

  wxString modifiers_name = KeyModifiersName(modifiers);
  wxString code_name = KeyCodeName(code);
  if (modifiers_name.IsEmpty()) {
    return code_name;
  } else {
    return modifiers_name + wxT("+") + code_name;
  }
}

int Key::GetAccelFlags() const {
  int flags = 0;

  int mod = modifiers();

#if defined(__WXMAC__)
  if ((mod & wxMOD_RAW_CONTROL) != 0) {
    flags |= wxACCEL_RAW_CTRL;
  }

  if ((mod & wxMOD_CMD) != 0) {
    flags |= wxACCEL_CMD;  // Or wxACCEL_CTRL
}
#else
  if ((mod & wxMOD_CONTROL) != 0) {
    flags |= wxACCEL_CTRL;
  }
#endif  // defined(__WXMAC__)

  if ((mod & wxMOD_ALT) != 0) {
    flags |= wxACCEL_ALT;
  }

  if ((mod & wxMOD_SHIFT) != 0) {
    flags |= wxACCEL_SHIFT;
  }

  return flags;
}

wxString Key::ToString() const {
#if JIL_ENABLE_LEADER_KEY
  wxString leader_key_str = SingleKeyName(data_ >> 16);
  wxString key_str = SingleKeyName(data_ & 0xffff);
  if (leader_key_str.IsEmpty()) {
    return key_str;
  } else {
    return leader_key_str + wxT(", ") + key_str;
  }
#else
  return SingleKeyName(data_);
#endif  // JIL_ENABLE_LEADER_KEY
}

}  // namespace editor
}  // namespace jil
