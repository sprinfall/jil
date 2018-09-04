#ifndef JIL_LEX_CONFIG_H_
#define JIL_LEX_CONFIG_H_

// Load lex file.

class wxString;

namespace editor {
class FtPlugin;
}  // namespace editor

namespace jil {

// Parse the lex file, set lex data to file type.
bool LoadLexFile(const wxString& lex_cfg_file, editor::FtPlugin* ft_plugin);

}  // namespace jil

#endif  // JIL_LEX_CONFIG_H_
