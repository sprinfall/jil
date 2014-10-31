#ifndef JIL_OPTION_CONFIG_H_
#define JIL_OPTION_CONFIG_H_
#pragma once

// Parse option settings.

class wxString;

namespace jil {

class Setting;

namespace editor {
class Options;
}  // namespace editor

class Options;

void ParseAppOptions(const Setting& setting, Options* options);

void ParseEditorOptions(const Setting& setting, editor::Options* options);

}  // namespace jil

#endif  // JIL_OPTION_CONFIG_H_
