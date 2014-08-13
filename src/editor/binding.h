#ifndef JIL_EDITOR_BINDING_H_
#define JIL_EDITOR_BINDING_H_
#pragma once

// Binding, or key mapping, binds key(s) to command.

#include <vector>
#include <list>
#include <map>
#include <utility>
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
  editor::TextFunc* func;
  int menu;
};

struct VoidCmd {
  std::string name;
  editor::VoidFunc* func;
  int menu;
};

class Binding {
 public:
  Binding();
  ~Binding();

  void AddTextCmd(const char* name, editor::TextFunc* func, int menu);
  void AddVoidCmd(const char* name, editor::VoidFunc* func, int menu);

  const TextCmd* GetTextCmdByName(const std::string& name) const;
  const VoidCmd* GetVoidCmdByName(const std::string& name) const;

  void BindTextFunc(TextFunc* text_func,
                    const std::vector<Key>& keys,
                    int modes,
                    int menu);

  void BindVoidFunc(VoidFunc* void_func,
                    const std::vector<Key>& keys,
                    int menu);

  TextFunc* GetTextFuncByMenu(int menu) const;

  VoidFunc* GetVoidFuncByMenu(int menu) const;

  TextFunc* GetTextFuncByKey(Key key, Mode mode) const;

  VoidFunc* GetVoidFuncByKey(Key key, int* menu = NULL) const;

  Key GetKeyByMenu(int menu) const;

  bool IsLeaderKey(Key key) const;

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

  typedef std::map<Key, TextFunc*> TextFuncMap;
  // Keep command functions of each mode in separate maps for
  // the convenience of searching.
  TextFuncMap normal_text_funcs_;
  TextFuncMap visual_text_funcs_;

  // Key -> (Void function, Menu)
  typedef std::map<Key, std::pair<VoidFunc*, int> > VoidFuncMap;
  VoidFuncMap void_funcs_;

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
