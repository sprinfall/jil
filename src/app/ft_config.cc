#include "app/ft_config.h"
#include <string>

#include "boost/algorithm/string.hpp"

#include "wx/log.h"
#include "wx/filename.h"

#include "editor/defs.h"
#include "editor/util.h"

#include "app/config.h"
#include "app/defs.h"
#include "app/i18n_strings.h"
#include "app/util.h"

namespace jil {

FtConfig::FtConfig() {
  // Plain Text file type.
  editor::FileType* ft_txt = new editor::FileType(kTxtFt);
  file_types_.push_back(ft_txt);

  // Files without extension name are considered as Plain Text file.
  fn_ext_ft_map_[wxEmptyString] = ft_txt;
  fn_ext_ft_map_[kTxtFtId] = ft_txt;
}

FtConfig::~FtConfig() {
  editor::ClearContainer(&file_types_);
  editor::ClearContainer(&internal_file_types_);
}

bool FtConfig::Load(const wxString& ft_config_file) {
  Config config;
  if (!config.Load(ft_config_file)) {
    wxLogError(wxT("Failed to load config file: %s"), ft_config_file);
    return false;
  }

  Setting root_setting = config.Root();

  Setting list_setting = root_setting.Get("list");
  if (!list_setting || list_setting.type() != Setting::kList) {
    return false;
  }

  int count = list_setting.size();

  for (int j = 0; j < count; ++j) {
    Setting setting = list_setting.Get(j);

    wxString id = wxString::FromAscii(setting.GetString("id"));
    wxString name = wxString::FromAscii(setting.GetString("name"));

    editor::FileType* ft = new editor::FileType(id, name);

    if (id.StartsWith(kInternalFtIdPrefix)) {
      internal_file_types_.push_back(ft);
    } else {
      file_types_.push_back(ft);
    }

    Setting fn_ext_setting = setting.Get("fn_ext");
    if (fn_ext_setting) {
      std::string fn_ext = fn_ext_setting.GetString();
      // File name extension should always be ASCII chars.
      MapToFt(fn_ext, ft, false, fn_ext_ft_map_);
    } else {
      Setting fn_setting = setting.Get("fn");
      if (fn_setting) {
        std::string fn = fn_setting.GetString();
        // File name itself could be non-ASCII chars.
        MapToFt(fn, ft, true, fn_ft_map_);
      }
    }
  }

  return true;
}

const editor::FileType& FtConfig::GetByFileName(const wxFileName& fn) const {
  wxString ext = fn.GetExt();

#if defined(__WXMSW__)
  ext.MakeLower();
#endif

  FileTypeMap::const_iterator it = fn_ext_ft_map_.find(ext);
  if (it == fn_ext_ft_map_.end()) {
    // No matching, use Plain Text file type.
    it = fn_ext_ft_map_.find(kTxtFtId);
  }

  // Check file name if it's Plain Text.
  if (it->second->id == kTxtFtId) {
    wxString name = fn.GetFullName();

#if defined(__WXMSW__)
    name.MakeLower();
#endif

    FileTypeMap::const_iterator it2 = fn_ft_map_.find(name);
    if (it2 != fn_ft_map_.end()) {
      it = it2;
    }
  }

  return *(it->second);
}

void FtConfig::MapToFt(const std::string& keys_str,
                       editor::FileType* ft,
                       bool utf8,
                       FileTypeMap& ft_map) {
  std::vector<std::string> keys_array;
  boost::split(keys_array, keys_str, boost::is_any_of(","), boost::token_compress_on);

  for (size_t i = 0; i < keys_array.size(); ++i) {
    wxString key;

    if (utf8) {
      key = wxString::FromUTF8(keys_array[i].c_str());
    } else {
      key = wxString::FromAscii(keys_array[i].c_str());
    }

    // Ignore case for file names on Windows.
    // TODO: It might also be necessary on Linux or Mac. Confirm it.
#if defined(__WXMSW__)
    key.MakeLower();
#endif

    if (ft_map.find(key) == ft_map.end()) {
      ft_map[key] = ft;
    } else {
      wxLogError(wxT("Can't map [%s] to file type [%s]."), key, ft->name);
    }
  }
}

}  // namespace jil
