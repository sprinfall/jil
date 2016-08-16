#ifndef JIL_THEME_CONFIG_H_
#define JIL_THEME_CONFIG_H_
#pragma once

#include "editor/theme.h"
#include "app/option.h"

// Load theme file.

class wxString;

namespace jil {

namespace editor {
class Style;
}

// \param theme_folder The root dir of a theme.
bool LoadThemeFile(const wxString& theme_folder,
                   IconSize icon_size,
                   editor::SharedTheme& theme,
                   editor::Style* style);

}  // namespace jil

#endif  // JIL_THEME_CONFIG_H_
