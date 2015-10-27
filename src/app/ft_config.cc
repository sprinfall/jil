#include "app/ft_config.h"
#include <string>
#include "boost/algorithm/string.hpp"
#include "wx/log.h"
#include "wx/filename.h"
#include "editor/defs.h"
#include "app/config.h"
#include "app/defs.h"
#include "app/i18n_strings.h"
#include "app/util.h"

namespace jil {

bool LoadFtConfigFile(const wxString& ft_file,
                      ExtFileTypeMap& ext_ft_map,
                      FileTypes& file_types,
                      FileTypes& internal_file_types) {
  Config config;
  if (!config.Load(ft_file)) {
    wxLogError(wxT("Failed to load config file: %s"), ft_file);
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
    std::string ext = setting.GetString("ext");

    editor::FileType* ft = new editor::FileType(id, name);

    if (id.StartsWith(kInternalFtIdPrefix)) {
      internal_file_types.push_back(ft);
    } else {
      file_types.push_back(ft);
    }

    // Map ext to file type.
    std::vector<std::string> ext_array;
    boost::split(ext_array, ext, boost::is_any_of(","), boost::token_compress_on);

    for (size_t i = 0; i < ext_array.size(); ++i) {
      wxString ext_key = wxString::FromAscii(ext_array[i].c_str());
      if (ext_ft_map.find(ext_key) == ext_ft_map.end()) {
        ext_ft_map[ext_key] = ft;
      } else {
        wxLogError(wxT("Can't map ext [%s] to file type [%s]."), ext_key, ft->name);
      }
    }
  }

  return true;
}

}  // namespace jil
