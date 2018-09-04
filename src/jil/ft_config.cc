#include "jil/ft_config.h"
#include <string>

#include "wx/log.h"
#include "wx/filename.h"

#include "editor/defs.h"
#include "editor/util.h"

#include "jil/config.h"
#include "jil/defs.h"
#include "jil/i18n_strings.h"
#include "jil/util.h"

namespace jil {

FtConfig::FtConfig() {
  // Plain Text file type.
  ft_txt_ = new editor::FileType(kTxtFt);
  file_types_.push_back(ft_txt_);

  // Files without extension name are considered as Plain Text file.
  fn_ext_ft_map_[wxEmptyString] = ft_txt_;
  fn_ext_ft_map_[kTxtFtId] = ft_txt_;
}

FtConfig::~FtConfig() {
  editor::ClearContainer(&file_types_);
  editor::ClearContainer(&internal_file_types_);
}

bool FtConfig::Load(const wxString& ft_cfg_file) {
  Config config;
  if (!config.Load(ft_cfg_file)) {
    wxLogError(wxT("Failed to load config file: %s"), ft_cfg_file);
    return false;
  }

  Setting root_setting = config.Root();


  LoadNameRules(root_setting);

  LoadPairRules(root_setting);


  return true;
}

const editor::FileType& FtConfig::GetByFileName(const wxFileName& fn) const {
  // Check file extension.
  editor::FileType* ft = ft_txt_;  // Plain Text as default.
  GetFtByFnExt(fn.GetExt(), &ft);

  // Check file name if it's Plain Text.
  if (ft->id == kTxtFtId) {
    GetFtByFn(fn.GetFullName(), &ft);
  }

  if (IsPaired(fn, &ft)) {
    //wxLogDebug(wxT("File '%s' is paired, ft: %s"), fn.GetFullName(), ft->id);
  }

  return *ft;
}

bool FtConfig::LoadNameRules(Setting& root_setting) {
  Setting list_setting = root_setting.Get("name");
  if (!list_setting || list_setting.type() != Setting::kList) {
    return false;
  }

  int count = list_setting.size();

  for (int i = 0; i < count; ++i) {
    Setting setting = list_setting.Get(i);

    wxString id = wxString::FromAscii(setting.GetString("id"));
    wxString name = wxString::FromAscii(setting.GetString("name"));

    editor::FileType* ft = new editor::FileType(id, name);

    if (id.StartsWith(kInternalFtIdPrefix)) {
      internal_file_types_.push_back(ft);
    } else {
      file_types_.push_back(ft);
    }

    // NOTE:
    // File name extension should always be ASCII chars.
    // File name itself could be non-ASCII chars.

    Setting fn_ext_setting = setting.Get("fn_ext");
    if (fn_ext_setting) {
      wxString fn_ext = fn_ext_setting.GetWxString(false);

#if defined(__WXMSW__)
      fn_ext.MakeLower();
#endif

      MapToFt(fn_ext, ft, fn_ext_ft_map_);

      continue;
    }

    Setting fn_setting = setting.Get("fn");
    if (fn_setting) {
      wxString fn = fn_setting.GetWxString(true);

#if defined(__WXMSW__)
      fn.MakeLower();
#endif

      MapToFt(fn, ft, fn_ft_map_);
    }
  }

  return true;
}

// Config example:
// pair = (
//     {
//         fn_ext = "h";
//         paired = "c,cpp,cc,cxx";
//     }
// );
bool FtConfig::LoadPairRules(Setting& root_setting) {
  Setting list_setting = root_setting.Get("pair");
  if (!list_setting || list_setting.type() != Setting::kList) {
    return false;
  }

  int count = list_setting.size();

  for (int i = 0; i < count; ++i) {
    Setting setting = list_setting.Get(i);

    wxString fn_ext = setting.GetWxString("fn_ext", false);
    wxString paired = setting.GetWxString("paired", false);

#if defined(__WXMSW__)
    fn_ext.MakeLower();
    paired.MakeLower();
#endif

    wxArrayString paired_fn_exts = wxSplit(paired, wxT(','));
    paired_fn_ext_map_[fn_ext] = paired_fn_exts;
  }

  return true;
}

bool FtConfig::LoadContentRules(Setting& root_setting) {
  return true;
}

void FtConfig::MapToFt(const wxString& keys_str,
                       editor::FileType* ft,
                       FileTypeMap& ft_map) {
  wxArrayString keys = wxSplit(keys_str, wxT(','));
  for (size_t i = 0; i < keys.size(); ++i) {
    wxString key = keys[i];
    if (ft_map.find(key) == ft_map.end()) {
      ft_map[key] = ft;
    } else {
      wxLogError(wxT("Can't map [%s] to file type [%s]."), key, ft->name);
    }
  }
}

bool FtConfig::GetFtByFnExt(const wxString& fn_ext, editor::FileType** ft) const {
  FileTypeMap::const_iterator it;

#if defined(__WXMSW__)
  it = fn_ext_ft_map_.find(fn_ext.Lower());
#else
  it = fn_ext_ft_map_.find(fn_ext);
#endif

  if (it != fn_ext_ft_map_.end()) {
    *ft = it->second;
    return true;
  }

  return false;
}

bool FtConfig::GetFtByFn(const wxString& fn, editor::FileType** ft) const {
  FileTypeMap::const_iterator it;

#if defined(__WXMSW__)
  it = fn_ft_map_.find(fn.Lower());
#else
  it = fn_ft_map_.find(fn);
#endif

  if (it != fn_ft_map_.end()) {
    *ft = it->second;
    return true;
  }

  return false;
}

bool FtConfig::IsPaired(const wxFileName& fn, editor::FileType** ft) const {
  wxString ext = fn.GetExt();

#if defined(__WXMSW__)
  ext.MakeLower();
#endif

  std::map<wxString, wxArrayString>::const_iterator it = paired_fn_ext_map_.find(ext);

  if (it == paired_fn_ext_map_.end()) {
    return false;
  }

  if (ft != NULL) {
    wxFileName paired_fn(fn);
    const wxArrayString& paired_exts = it->second;

    for (const wxString& paired_ext : paired_exts) {
      paired_fn.SetExt(paired_ext);

      if (paired_fn.FileExists()) {
        // Find the paired file.
        GetFtByFnExt(paired_ext, ft);
        break;
      }
    }
  }

  return true;
}

}  // namespace jil
