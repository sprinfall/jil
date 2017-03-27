#include "app/binding_config.h"
#include <vector>
#include <cctype>
#include "wx/log.h"
#include "editor/binding.h"
#include "app/app.h"
#include "app/config.h"
#include "app/id.h"
#include "app/book_frame.h"
#include "app/compile_config.h"

namespace jil {

using namespace editor;

namespace {

int ParseModifier(const std::string& modifier_str) {
  if (modifier_str == "ctrl") {
#if defined(__WXMAC__)
    return wxMOD_RAW_CONTROL;
#else
    return wxMOD_CONTROL;
#endif
  } else if (modifier_str == "alt") {
    return wxMOD_ALT;
  } else if (modifier_str == "shift") {
    return wxMOD_SHIFT;
#if defined(__WXMAC__)
  } else if (modifier_str == "cmd") {
    return wxMOD_CONTROL;
#endif
  } else {
    return wxMOD_NONE;
  }
}

// F1, F2, etc.
int ParseF(const std::string& keycode_str) {
  assert(keycode_str.size() > 1);

  if (keycode_str[0] != 'f' && keycode_str[0] != 'F') {
    return 0;
  }

  int num = std::atoi(&keycode_str[1]);
  if (num < 1 || num > 12) {  // Only support F1 ~ F2.
    return 0;
  }

  return WXK_F1 + (num - 1);
}

#define kKeyCount 16

// SORTED special key names.
const char* kSortedKeyNames[kKeyCount] = {
  "back",
  "delete",
  "down",
  "end",
  "enter",
  "escape",
  "home",
  "insert",
  "left",
  "pagedown",
  "pageup",
  "return",
  "right",
  "space",
  "tab",
  "up",
};

// Special key codes SORTED by name.
const wxKeyCode kSortedKeyCodes[kKeyCount] = {
  WXK_BACK,
  WXK_DELETE,
  WXK_DOWN,
  WXK_END,
  WXK_RETURN,
  WXK_ESCAPE,
  WXK_HOME,
  WXK_INSERT,
  WXK_LEFT,
  WXK_PAGEDOWN,
  WXK_PAGEUP,
  WXK_RETURN,
  WXK_RIGHT,
  WXK_SPACE,
  WXK_TAB,
  WXK_UP,
};

size_t IndexKeyName(const char* keyname) {
  size_t i = 0;
  size_t j = kKeyCount;

  while (j > i) {
    size_t m = i + (j - i) / 2;
    int cmp = strcmp(keyname, kSortedKeyNames[m]);
    if (cmp == 0) {
      return m;
    } else if (cmp < 0) {
      j = m;
    } else {
      i = m + 1;
    }
  }

  return kKeyCount;
}

// "s" -> S, "F1" -> WXK_F1, home -> WXK_HOME, etc.
int ParseKeycode(const std::string& keycode_str, int* modifiers) {
  assert(modifiers != NULL);

  if (keycode_str.empty()) {
    return 0;
  }

  if (keycode_str.size() == 1) {
    char keycode = keycode_str[0];
    if (std::isalpha(keycode) != 0) {
      keycode = toupper(keycode);  // Virtual key is in upper case.
    } else {
      // Example: $ -> Shift+4
      size_t pos = editor::kShiftChars.find(keycode);
      if (pos != std::string::npos) {
        keycode = editor::kNonShiftChars[pos];
        *modifiers |= wxMOD_SHIFT;
      }
    }

    return keycode;
  }

  int keycode = ParseF(keycode_str);
  if (keycode != 0) {
    return keycode;
  }

  size_t i = IndexKeyName(keycode_str.c_str());
  if (i != kKeyCount) {
    return kSortedKeyCodes[i];
  }

  return 0;
}

// A command can be bound with multiple keys, separated with semicolon.
// Examples:
//  - delete;back ('delete' and 'back' are bound to the same command)
//  - ctrl+;;ctrl+i
// For the second example, the first simicolon is the keycode while the second
// semicolon is the key delimiter. That's why we don't use a general tokenizer.
void SplitKeys(const std::string& keys_str, std::vector<std::string>* splited_keys_str) {
  size_t p = 0;
  size_t i = 0;
  for (; i < keys_str.size(); ++i) {
    if (keys_str[i] == ';') {
      if (i > p) {
        // A semicolon after plus is not a separator.
        if (keys_str[i - 1] != '+') {
          splited_keys_str->push_back(keys_str.substr(p, i - p));
          p = i + 1;
        }
      }
    }
  }
  if (i > p) {
    splited_keys_str->push_back(keys_str.substr(p, i - p));
  }
}

bool ParseKeyStroke(const std::string& key_str, int* code, int* modifiers) {
  size_t offset = 0;
  while (true) {
    size_t plus = key_str.find_first_of('+', offset);
    if (plus != std::string::npos) {
      *modifiers = *modifiers | ParseModifier(key_str.substr(offset, plus - offset));
    } else {
      *code = ParseKeycode(key_str.substr(offset), modifiers);
      if (*code == 0) {
        return false;
      }
      break;
    }
    offset = plus + 1;
  }
  return true;
}

bool ParseKey(const std::string& key_str, Key* key) {
#if JIL_ENABLE_LEADER_KEY
  int leader_code = 0;
  int leader_modifiers = 0;
#endif  // JIL_ENABLE_LEADER_KEY

  int code = 0;
  int modifiers = 0;

#if JIL_ENABLE_LEADER_KEY
  // Check leader key.
  size_t comma = key_str.find_first_of(',');
  if (comma != std::string::npos) {
    if (comma > 0 && key_str[comma - 1] == '+') {
      comma = key_str.find_first_of(',', comma + 1);
    }
  }

  if (comma != std::string::npos) {
    if (!ParseKeyStroke(key_str.substr(0, comma), &leader_code, &leader_modifiers)) {
      return false;
    }
    if (!ParseKeyStroke(key_str.substr(comma + 1), &code, &modifiers)) {
      return false;
    }
  } else {  // No leader key.
#endif  // JIL_ENABLE_LEADER_KEY
    if (!ParseKeyStroke(key_str, &code, &modifiers)) {
      return false;
    }
#if JIL_ENABLE_LEADER_KEY
  }
#endif  // JIL_ENABLE_LEADER_KEY

#if JIL_ENABLE_LEADER_KEY
  key->Set(leader_code, leader_modifiers, code, modifiers);
#else
  key->Set(code, modifiers);
#endif  // JIL_ENABLE_LEADER_KEY

  return true;
}

std::vector<Key> ParseKeys(const std::string& keys_str) {
  std::vector<Key> keys;
  std::vector<std::string> splited_keys_str;
  SplitKeys(keys_str, &splited_keys_str);

  Key key;
  for (size_t i = 0; i < splited_keys_str.size(); ++i) {
    if (!ParseKey(splited_keys_str[i], &key)) {
      keys.clear();
      break;
    } else {
      keys.push_back(key);
    }
    key.Reset();
  }

  return keys;
}

int ParseMode(const std::string& mode_str) {
  if (mode_str.empty()) {
    return kAllModes;
  } else if (mode_str == "normal") {
    return kNormalMode;
  } else if (mode_str == "visual") {
    return kVisualMode;
  } else {
    return kAllModes;
  }
}

}  // namespace

BindingConfig::BindingConfig(Binding* binding)
    : binding_(binding) {
}

BindingConfig::~BindingConfig() {
}

bool BindingConfig::Load(const wxString& binding_cfg_file) {
  Config config;
  if (!config.Load(binding_cfg_file)) {
    wxLogDebug(wxT("Failed to load binding file: %s"), binding_cfg_file.c_str());
    return false;
  }

  Setting root_setting = config.Root();
  int binding_list_num = root_setting.size();

  for (int i = 0; i < binding_list_num; ++i) {
    Setting list_setting = root_setting.Get(i);

    if (!list_setting || list_setting.type() != Setting::kList) {
      wxLogDebug(wxT("Binding items should be in a list."));
      continue;
    }

    int nr_of_binding = list_setting.size();
    for (int j = 0; j < nr_of_binding; ++j) {
      ParseBindingItem(list_setting.Get(j));
    }
  }

  return true;
}

bool BindingConfig::ParseBinding(const std::string& cmd, const std::string& key, int modes) {
  std::vector<Key> keys = ParseKeys(key);
  if (keys.empty()) {
    return false;
  }

  if (!binding_->BindKeys(cmd, keys, modes)) {
    wxLogWarning(wxT("Unknown command: %s"), cmd.c_str());
    return false;
  }

  return true;
}

bool BindingConfig::ParseBindingItem(const Setting& setting) {
  std::string cmd = setting.GetString("cmd");
  std::string key = setting.GetString("key");
  int mode = ParseMode(setting.GetString("mode"));
  return ParseBinding(cmd, key, mode);
}

}  // namespace jil
