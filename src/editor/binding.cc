#include "editor/binding.h"

namespace editor {

Binding::~Binding() {
  for (size_t i = 0; i < text_cmds_.size(); ++i) {
    delete text_cmds_[i].func;
  }

  for (size_t i = 0; i < void_cmds_.size(); ++i) {
    delete void_cmds_[i].func;
  }
}

void Binding::AddTextCmd(const std::string& name, TextFunc* func, int menu) {
  TextCmd text_cmd;
  text_cmd.name = name;
  text_cmd.func = func;
  text_cmd.menu = menu;

  text_cmds_.push_back(text_cmd);
}

void Binding::AddVoidCmd(const std::string& name, VoidFunc* func, int menu) {
  VoidCmd void_cmd;
  void_cmd.name = name;
  void_cmd.func = func;
  void_cmd.menu = menu;

  void_cmds_.push_back(void_cmd);
}

bool Binding::BindKeys(const std::string& name, const std::vector<Key>& keys,
                       int modes) {
#if JIL_ENABLE_LEADER_KEY
  // Save leader key.
  for (size_t i = 0; i < keys.size(); ++i) {
    Key leader_key = keys[i].leader();
    if (!leader_key.IsEmpty() && !IsLeaderKey(leader_key)) {
      leader_keys_.push_back(leader_key);
    }
  }
#endif  // JIL_ENABLE_LEADER_KEY

  // Match text command.
  TextCmd* text_cmd = GetTextCmdByName(name);
  if (text_cmd != NULL) {
    text_cmd->keys = keys;

    for (size_t i = 0; i < keys.size(); ++i) {
      if ((modes & kNormalMode) != 0) {
        normal_text_keys_[keys[i]] = std::make_pair(text_cmd->func,
                                                    text_cmd->menu);
      }

      if ((modes & kVisualMode) != 0) {
        visual_text_keys_[keys[i]] = std::make_pair(text_cmd->func,
                                                    text_cmd->menu);
      }
    }

    return true;
  }

  // Match void command.
  VoidCmd* void_cmd = GetVoidCmdByName(name);
  if (void_cmd != NULL) {
    void_cmd->keys = keys;

    for (size_t i = 0; i < keys.size(); ++i) {
      void_keys_[keys[i]] = std::make_pair(void_cmd->func, void_cmd->menu);
    }

    return true;
  }

  return false;
}

void Binding::BindMenus() {
  Key key;

  for (size_t i = 0; i < text_cmds_.size(); ++i) {
    TextCmd& text_cmd = text_cmds_[i];
    if (text_cmd.menu != 0) {
      if (text_cmd.keys.empty()) {
        key.Reset();
      } else {
        key = text_cmd.keys[0];
      }
      text_menus_[text_cmd.menu] = std::make_pair(text_cmd.func, key);
    }
  }

  for (size_t i = 0; i < void_cmds_.size(); ++i) {
    VoidCmd& void_cmd = void_cmds_[i];
    if (void_cmd.menu != 0) {
      if (void_cmd.keys.empty()) {
        key.Reset();
      } else {
        key = void_cmd.keys[0];
      }
      void_menus_[void_cmd.menu] = std::make_pair(void_cmd.func, key);
    }
  }
}

TextFunc* Binding::GetTextFuncByMenu(int menu) const {
  TextMenuMap::const_iterator it = text_menus_.find(menu);
  if (it == text_menus_.end()) {
    return NULL;
  }
  return it->second.first;
}

VoidFunc* Binding::GetVoidFuncByMenu(int menu) const {
  VoidMenuMap::const_iterator it = void_menus_.find(menu);
  if (it == void_menus_.end()) {
    return NULL;
  }
  return it->second.first;
}

TextFunc* Binding::GetTextFuncByKey(Key key, Mode mode, int* menu) const {
  const TextKeyMap& text_keys =
      mode == kNormalMode ? normal_text_keys_ : visual_text_keys_;

  TextKeyMap::const_iterator it = text_keys.find(key);
  if (it == text_keys.end()) {
    return NULL;
  }

  const std::pair<TextFunc*, int>& value = it->second;
  if (menu != NULL) {
    *menu = value.second;
  }

  return value.first;
}

VoidFunc* Binding::GetVoidFuncByKey(Key key, int* menu) const {
  VoidKeyMap::const_iterator it = void_keys_.find(key);
  if (it == void_keys_.end()) {
    return NULL;
  }

  const std::pair<VoidFunc*, int>& value = it->second;
  if (menu != NULL) {
    *menu = value.second;
  }

  return value.first;
}

Key Binding::GetKeyByMenu(int menu) const {
  TextMenuMap::const_iterator text_it = text_menus_.find(menu);
  if (text_it != text_menus_.end()) {
    return text_it->second.second;
  }

  VoidMenuMap::const_iterator void_it = void_menus_.find(menu);
  if (void_it != void_menus_.end()) {
    return void_it->second.second;
  }

  return Key();
}

#if JIL_ENABLE_LEADER_KEY
bool Binding::IsLeaderKey(Key key) const {
  return std::find(leader_keys_.begin(), leader_keys_.end(), key) !=
         leader_keys_.end();
}
#endif  // JIL_ENABLE_LEADER_KEY

TextCmd* Binding::GetTextCmdByName(const std::string& name) {
  for (size_t i = 0; i < text_cmds_.size(); ++i) {
    if (name == text_cmds_[i].name) {
      return &text_cmds_[i];
    }
  }
  return NULL;
}

VoidCmd* Binding::GetVoidCmdByName(const std::string& name) {
  for (size_t i = 0; i < void_cmds_.size(); ++i) {
    if (name == void_cmds_[i].name) {
      return &void_cmds_[i];
    }
  }
  return NULL;
}

}  // namespace editor
