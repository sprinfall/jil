#include "editor/binding.h"

namespace jil {
namespace editor {

Binding::Binding() {
}

Binding::~Binding() {
  for (size_t i = 0; i < text_cmds_.size(); ++i) {
    delete text_cmds_[i].func;
  }

  for (size_t i = 0; i < void_cmds_.size(); ++i) {
    delete void_cmds_[i].func;
  }
}

void Binding::AddTextCmd(const char* name, editor::TextFunc* func, int menu) {
  TextCmd text_cmd = { name, func, menu };
  text_cmds_.push_back(text_cmd);
}

void Binding::AddVoidCmd(const char* name, editor::VoidFunc* func, int menu) {
  VoidCmd void_cmd = { name, func, menu };
  void_cmds_.push_back(void_cmd);
}

const TextCmd* Binding::GetTextCmdByName(const std::string& name) const {
  for (size_t i = 0; i < text_cmds_.size(); ++i) {
    if (name == text_cmds_[i].name) {
      return &text_cmds_[i];
    }
  }
  return NULL;
}

const VoidCmd* Binding::GetVoidCmdByName(const std::string& name) const {
  for (size_t i = 0; i < void_cmds_.size(); ++i) {
    if (name == void_cmds_[i].name) {
      return &void_cmds_[i];
    }
  }
  return NULL;
}

void Binding::BindTextFunc(TextFunc* text_func,
                           const std::vector<Key>& keys,
                           int modes,
                           int menu) {
  assert(!keys.empty());

  for (size_t i = 0; i < keys.size(); ++i) {
    if ((modes & kNormalMode) != 0) {
      normal_text_funcs_[keys[i]] = text_func;
    }
    if ((modes & kVisualMode) != 0) {
      visual_text_funcs_[keys[i]] = text_func;
    }

    Key leader_key = keys[i].leader();
    if (!leader_key.IsEmpty() && !IsLeaderKey(leader_key)) {
      leader_keys_.push_back(leader_key);
    }
  }

  if (menu != 0) {
    text_menus_[menu] = std::make_pair(text_func, keys[0]);
  }
}

void Binding::BindVoidFunc(VoidFunc* void_func,
                           const std::vector<Key>& keys,
                           int menu) {
  assert(!keys.empty());

  for (size_t i = 0; i < keys.size(); ++i) {
    void_funcs_[keys[i]] = std::make_pair(void_func, menu);

    Key leader_key = keys[i].leader();
    if (!leader_key.IsEmpty() && !IsLeaderKey(leader_key)) {
      leader_keys_.push_back(leader_key);
    }
  }

  if (menu != 0) {
    void_menus_[menu] = std::make_pair(void_func, keys[0]);
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

TextFunc* Binding::GetTextFuncByKey(Key key, Mode mode) const {
  const TextFuncMap& text_funcs = mode == kNormalMode ?
                                  normal_text_funcs_ : visual_text_funcs_;
  TextFuncMap::const_iterator it = text_funcs.find(key);
  if (it == text_funcs.end()) {
    return NULL;
  }
  return it->second;
}

VoidFunc* Binding::GetVoidFuncByKey(Key key, int* menu) const {
  VoidFuncMap::const_iterator it = void_funcs_.find(key);
  if (it == void_funcs_.end()) {
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

bool Binding::IsLeaderKey(Key key) const {
  return std::find(leader_keys_.begin(),
                   leader_keys_.end(),
                   key) != leader_keys_.end();
}

}  // namespace editor
}  // namespace jil
