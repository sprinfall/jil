#ifndef JIL_EDITOR_BINDING_H_
#define JIL_EDITOR_BINDING_H_
#pragma once

// Binding, or key mapping, binds key(s) to command.

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "boost/function.hpp"
#include "editor/key.h"
#include "editor/text_func.h"

namespace jil {
namespace editor {

// Void function interface.
class VoidFunc {
public:
  virtual void Exec() = 0;
};

// Use with predefined void functions.
class VoidFuncWrap : public VoidFunc {
  typedef void(*Func)();

public:
  VoidFuncWrap(Func func) : func_(func) {
  }

protected:
  virtual void Exec() override {
    if (func_ != NULL) {
      (*func_)();
    }
  }

private:
  Func func_;
};

struct TextCmd {
  std::string name;
  TextFunc* func;
  std::vector<Key> keys;
  int menu;
  int modes;
};

struct VoidCmd {
  std::string name;
  VoidFunc* func;
  std::vector<Key> keys;
  int menu;
};

class Binding {
public:
  Binding();
  ~Binding();

  void AddTextCmd(const char* name, TextFunc* func, int menu);
  void AddVoidCmd(const char* name, VoidFunc* func, int menu);

  // Bind the keys to the command with the given name.
  bool BindKeys(const std::string& name,
                const std::vector<Key>& keys,
                int modes);

  // Build the map from menu to (function, key) pair.
  void BindMenus();

  // Get text func by menu id.
  // Used to execute text func by clicking menu item.
  TextFunc* GetTextFuncByMenu(int menu) const;

  // Get void func by menu id.
  // Used to execute void func by clicking menu item.
  VoidFunc* GetVoidFuncByMenu(int menu) const;

  // Get text func by key in a given mode.
  // Used to execute text func by pressing keys.
  TextFunc* GetTextFuncByKey(Key key, Mode mode, int* menu = NULL) const;

  // Get void func by key in a given mode.
  // Used to execute void func by pressing keys.
  // \param menu Can be used to determine if the menu is enabled or not.
  //   If the menu is disabled, the void func shouldn't be executed.
  VoidFunc* GetVoidFuncByKey(Key key, int* menu = NULL) const;

  // Get the keys by menu id.
  // Used when create menu item.
  Key GetKeyByMenu(int menu) const;

  bool IsLeaderKey(Key key) const;

private:
  TextCmd* GetTextCmdByName(const std::string& name);
  VoidCmd* GetVoidCmdByName(const std::string& name);

private:
  // Command tables.
  // NOTE: Using vector and linear time search is more efficient than map.
  // So don't try to use map.
  // Time of constructing command tables and loading binding config:
  // (Test with release version)
  // - Use vector: 0ms
  // - Use map: 1ms
  std::vector<TextCmd> text_cmds_;
  std::vector<VoidCmd> void_cmds_;

  // Keep text functions of each mode in separate maps for
  // the convenience of searching.
  typedef std::map<Key, std::pair<TextFunc*, int> > TextKeyMap;
  TextKeyMap normal_text_keys_;
  TextKeyMap visual_text_keys_;

  // Key -> (Void function, Menu)
  typedef std::map<Key, std::pair<VoidFunc*, int> > VoidKeyMap;
  VoidKeyMap void_keys_;

  // Menu -> (Text function, Key)
  typedef std::map<int, std::pair<TextFunc*, Key> > TextMenuMap;
  TextMenuMap text_menus_;

  // Menu -> (Void function, Key)
  typedef std::map<int, std::pair<VoidFunc*, Key> > VoidMenuMap;
  VoidMenuMap void_menus_;

  // Store leader keys separately for fast checking.
  std::vector<Key> leader_keys_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_BINDING_H_
