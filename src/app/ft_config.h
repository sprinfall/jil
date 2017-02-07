#ifndef JIL_FT_CONFIG_H_
#define JIL_FT_CONFIG_H_
#pragma once

// Load file types config file.

#include <map>
#include <vector>
#include <cassert>

#include "editor/defs.h"

class wxFileName;
class wxString;

namespace jil {

namespace editor {
class FileType;
}  // namespace editor

typedef std::vector<editor::FileType*> FileTypes;
typedef std::map<wxString, editor::FileType*> FileTypeMap;

// File type configuration.
class FtConfig {
public:
  FtConfig();
  ~FtConfig();

  bool Load(const wxString& ft_config_file);

  const FileTypes& file_types() const {
    return file_types_;
  }

  const FileTypes& internal_file_types() const {
    return internal_file_types_;
  }

  size_t GetCount() const {
    return file_types_.size();
  }

  const editor::FileType* GetByIndex(size_t index) const {
    assert(index < file_types_.size());
    return file_types_[index];
  }

  const editor::FileType& GetByFileName(const wxFileName& fn) const;

private:
  void MapToFt(const std::string& keys_str,
               editor::FileType* ft,
               FileTypeMap& ft_map);

private:
  // File name extension to file type map.
  // E.g., "cpp" -> C++
  FileTypeMap fn_ext_ft_map_;

  // File name to file type map.
  // E.g., "CMakeLists.txt" -> CMake
  FileTypeMap fn_ft_map_;

  FileTypes file_types_;

  // The ID of internal file types always starts with "jil-".
  // Internal file types won't be displayed to the end user.
  FileTypes internal_file_types_;
};

}  // namespace jil

#endif  // JIL_LEX_CONFIG_H_
