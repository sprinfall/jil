#ifndef JIL_THEME_CONFIG_H_
#define JIL_THEME_CONFIG_H_

#include "editor/theme.h"

#include "jil/option.h"

// Load theme file.

class wxString;

namespace editor {
class Style;
}

namespace jil {

// \param theme_folder The root dir of a theme.
bool LoadThemeFile(const wxString& theme_folder,
                   Resolution icon_size,
                   editor::SharedTheme& theme,
                   editor::Style* style);

}  // namespace jil

#endif  // JIL_THEME_CONFIG_H_
