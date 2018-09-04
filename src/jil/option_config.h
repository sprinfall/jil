#ifndef JIL_OPTION_CONFIG_H_
#define JIL_OPTION_CONFIG_H_

#include <string>

#include "wx/string.h"

#include "jil/option.h"

// Parse option settings.

namespace editor {
class Options;
}  // namespace editor

namespace jil {

class Setting;

class Options;

std::string ResolutionToStr(Resolution resolution);

bool LoadGlobalOptionsFile(const wxString& cfg_file, Options* options);
bool SaveGlobalOptionsFile(const wxString& cfg_file, const Options& options);

bool LoadEditorOptionsFile(const wxString& cfg_file, editor::Options* options);
bool SaveEditorOptionsFile(const wxString& cfg_file, const editor::Options& options);

}  // namespace jil

#endif  // JIL_OPTION_CONFIG_H_
