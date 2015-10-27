#ifndef JIL_FT_CONFIG_H_
#define JIL_FT_CONFIG_H_
#pragma once

// Load file types config file.

#include <map>
#include <vector>

class wxString;

namespace jil {

namespace editor {
class FileType;
}  // namespace editor

typedef std::vector<editor::FileType*> FileTypes;

// Example: wxT("cpp") -> { "cpp", wxT("C++") }
typedef std::map<wxString, editor::FileType*> ExtFileTypeMap;

bool LoadFtConfigFile(const wxString& ft_file,
                      ExtFileTypeMap& ext_ft_map,
                      FileTypes& file_types,
                      FileTypes& internal_file_types);

}  // namespace jil

#endif  // JIL_LEX_CONFIG_H_
