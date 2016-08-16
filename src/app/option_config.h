#ifndef JIL_OPTION_CONFIG_H_
#define JIL_OPTION_CONFIG_H_
#pragma once

#include <string>
#include "wx/string.h"
#include "app/option.h"

// Parse option settings.

namespace jil {

class Setting;

namespace editor {
class Options;
}  // namespace editor

class Options;

std::string IconSizeToStr(IconSize icon_size);

bool LoadGlobalOptionsFile(const wxString& file, Options* options);
bool SaveGlobalOptionsFile(const wxString& file, const Options& options);

bool LoadEditorOptionsFile(const wxString& file, editor::Options* options);
bool SaveEditorOptionsFile(const wxString& file, const editor::Options& options);

}  // namespace jil

#endif  // JIL_OPTION_CONFIG_H_
