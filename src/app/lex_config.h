#ifndef JIL_LEX_CONFIG_H_
#define JIL_LEX_CONFIG_H_
#pragma once

// Load lex file.

class wxString;

namespace jil {

namespace editor {
class FtPlugin;
}

// Parse the lex file, set lex data to file type.
bool LoadLexFile(const wxString& lex_file, editor::FtPlugin* ft_plugin);

}  // namespace jil

#endif  // JIL_LEX_CONFIG_H_
