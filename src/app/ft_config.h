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
}

class Setting;

typedef std::vector<editor::FileType*> FileTypes;
typedef std::map<wxString, editor::FileType*> FileTypeMap;

// File type configuration.
class FtConfig {
public:
  FtConfig();
  ~FtConfig();

  bool Load(const wxString& ft_cfg_file);

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
  // Load rules determining file type by file name and extension.
  bool LoadNameRules(Setting& root_setting);

  // Load rules determining file type by paired file.
  bool LoadPairRules(Setting& root_setting);

  // Load rules determining file type by file content.
  bool LoadContentRules(Setting& root_setting);

  void MapToFt(const wxString& keys_str,
               editor::FileType* ft,
               FileTypeMap& ft_map);

  // Get file type by file name extension.
  bool GetFtByFnExt(const wxString& fn_ext, editor::FileType** ft) const;

  // Get file type by file name.
  bool GetFtByFn(const wxString& fn, editor::FileType** ft) const;

  // Check if the given file has any paired file.
  bool IsPaired(const wxFileName& fn, editor::FileType** ft = NULL) const;

private:
  // Cache Plain Text file type to avoid finding every time.
  editor::FileType* ft_txt_;

  FileTypes file_types_;

  // The ID of internal file types always starts with "jil-".
  // Internal file types won't be displayed to the end user.
  FileTypes internal_file_types_;

  // File name extension to file type map.
  // E.g., "cpp" -> C++
  FileTypeMap fn_ext_ft_map_;

  // File name to file type map.
  // E.g., "CMakeLists.txt" -> CMake
  FileTypeMap fn_ft_map_;

  // E.g., "h" -> [ "c", "cc", "cpp", "cxx" ]
  std::map<wxString, wxArrayString> paired_fn_ext_map_;
};

}  // namespace jil

#endif  // JIL_LEX_CONFIG_H_
