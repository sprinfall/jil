#ifndef JIL_BINDING_CONFIG_H_
#define JIL_BINDING_CONFIG_H_

// Parse binding config.

#include <string>
#include <vector>

#include "editor/defs.h"

namespace editor {
class Binding;
}  // namespace editor

namespace jil {

class Setting;

class BindingConfig {
public:
  explicit BindingConfig(editor::Binding* binding);
  ~BindingConfig();

  bool Load(const wxString& binding_cfg_file);

private:
  // Example: "delete.word.next", "ctrl+delete", kNormalMode
  bool ParseBinding(const std::string& cmd, const std::string& key, int modes);

  bool ParseBindingItem(const Setting& setting);

private:
  editor::Binding* binding_;
};

}  // namespace jil

#endif  // JIL_BINDING_CONFIG_H_
