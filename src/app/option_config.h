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

bool LoadGlobalOptionsFile(const wxString& file, Options* options);

bool LoadEditorOptionsFile(const wxString& file, editor::Options* options);
bool SaveEditorOptionsFile(const wxString& file, const editor::Options& options);

}  // namespace jil

#endif  // JIL_OPTION_CONFIG_H_
