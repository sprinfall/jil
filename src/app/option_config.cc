#include "app/option_config.h"
#include <string>
#include <vector>
#include "boost/algorithm/string.hpp"
#include "uchardet/nscore.h"
#include "uchardet/nsUniversalDetector.h"
#include "wx/log.h"
#include "wx/filename.h"
#include "wx/settings.h"
#include "editor/util.h"
#include "editor/option.h"
#include "app/config.h"
#include "app/option.h"
#include "app/font_util.h"

namespace jil {

////////////////////////////////////////////////////////////////////////////////
// Helper functions.

static int ParseCjk(const std::string& cjk) {
  if (cjk.empty()) {
    return 0;
  }

  int cjk_filters = 0;

  std::vector<std::string> cjk_values;
  boost::split(cjk_values,
               cjk,
               boost::is_any_of(", "),
               boost::token_compress_on);

  for (size_t i = 0; i < cjk_values.size(); ++i) {
    if (cjk_values[i] == "cs") {
      cjk_filters |= NS_FILTER_CHINESE_SIMPLIFIED;
    } else if (cjk_values[i] == "ct") {
      cjk_filters |= NS_FILTER_CHINESE_TRADITIONAL;
    } else if (cjk_values[i] == "c") {
      cjk_filters |= (NS_FILTER_CHINESE_SIMPLIFIED |
                      NS_FILTER_CHINESE_TRADITIONAL);
    } else if (cjk_values[i] == "j") {
      cjk_filters |= NS_FILTER_JAPANESE;
    } else if (cjk_values[i] == "k") {
      cjk_filters |= NS_FILTER_KOREAN;
    }
  }

  return cjk_filters;
}

static int AdjustCjkByLocale(int cjk_filters) {
  wxLocale locale;
  locale.Init();
  int lang = locale.GetLanguage();

  if ((cjk_filters & NS_FILTER_CHINESE_SIMPLIFIED) == 0) {
    if (lang == wxLANGUAGE_CHINESE || lang == wxLANGUAGE_CHINESE_SIMPLIFIED) {
      cjk_filters |= NS_FILTER_CHINESE_SIMPLIFIED;
    }
  }

  if ((cjk_filters & NS_FILTER_CHINESE_TRADITIONAL) == 0) {
    if (lang == wxLANGUAGE_CHINESE ||
        lang == wxLANGUAGE_CHINESE_TRADITIONAL ||
        lang == wxLANGUAGE_CHINESE_HONGKONG ||
        lang == wxLANGUAGE_CHINESE_MACAU ||
        lang == wxLANGUAGE_CHINESE_SINGAPORE ||
        lang == wxLANGUAGE_CHINESE_TAIWAN) {
      cjk_filters |= NS_FILTER_CHINESE_TRADITIONAL;
    }
  }

  if ((cjk_filters & NS_FILTER_JAPANESE) == 0 && lang == wxLANGUAGE_JAPANESE) {
    cjk_filters |= NS_FILTER_JAPANESE;
  }

  if ((cjk_filters & NS_FILTER_KOREAN) == 0 && lang == wxLANGUAGE_KOREAN) {
    cjk_filters |= NS_FILTER_KOREAN;
  }

  return cjk_filters;
}

static Setting GetSetting(const SettingMap& settings,
                          const char* key,
                          Setting::Type type) {
  SettingMap::const_iterator it = settings.find(key);
  if (it != settings.end() && it->second.type() == type) {
    return it->second;
  }
  return Setting();
}

static bool GetString(const SettingMap& settings,
                      const char* key,
                      std::string* value) {
  Setting setting = GetSetting(settings, key, Setting::kString);
  if (setting) {
    *value = setting.GetString();
    return true;
  }
  return false;
}

static bool GetWString(const SettingMap& settings,
                       const char* key,
                       std::wstring* value) {
  Setting setting = GetSetting(settings, key, Setting::kString);
  if (setting) {
    const char* str = setting.GetString();
    // Assume that the string is pure ascii.
    *value = std::wstring(str, str + strlen(str));
    return true;
  }
  return false;
}

static bool GetWxString(const SettingMap& settings,
                        const char* key,
                        wxString* value) {
  Setting setting = GetSetting(settings, key, Setting::kString);
  if (setting) {
    *value = wxString::FromUTF8(setting.GetString());
    return true;
  }
  return false;
}

static bool GetBool(const SettingMap& settings, const char* key, bool* value) {
  Setting setting = GetSetting(settings, key, Setting::kBool);
  if (setting) {
    *value = setting.GetBool();
    return true;
  }
  return false;
}

static bool GetInt(const SettingMap& settings, const char* key, int* value) {
  Setting setting = GetSetting(settings, key, Setting::kInt);
  if (setting) {
    *value = setting.GetInt();
    return true;
  }
  return false;
}

static void GetOptionTable(Setting setting, editor::OptionTable* option_table) {
  if (!setting) {
    return;
  }

  int size = setting.size();

  for (int i = 0; i < size; ++i) {
    std::string key = setting[i].name();
    editor::OptionValue value;

    int type = setting[i].type();

    if (type == Setting::kBool) {
      value = editor::OptionValue(setting[i].GetBool());
    } else if (type == Setting::kInt) {
      value = editor::OptionValue(setting[i].GetInt());
    } else if (type == Setting::kString) {
      value = editor::OptionValue(std::string(setting[i].GetString()));
    }

    if (!value.IsEmpty()) {
      option_table->push_back(std::make_pair(key, value));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void ParseAppOptions(const Setting& setting, Options* options) {
  SettingMap setting_map;
  setting.AsMap(&setting_map);

  // CJK
  std::string cjk_str;
  GetString(setting_map, CJK, &cjk_str);
  if (!cjk_str.empty()) {
    options->cjk_filters = ParseCjk(cjk_str);
  }
  options->cjk_filters = AdjustCjkByLocale(options->cjk_filters);

  // File encoding
  std::string fenc_str;
  GetString(setting_map, FILE_ENCODING, &fenc_str);
  if (fenc_str.empty()) {
    fenc_str = "utf-8";
  }
  options->file_encoding = editor::EncodingFromName(fenc_str);

  // Fonts
  Setting fonts_setting = GetSetting(setting_map, FONTS, Setting::kGroup);
  if (fonts_setting) {
    options->fonts[kFont_Text] = fonts_setting.GetFont("text");
    options->fonts[kFont_Tab] = fonts_setting.GetFont("tab");
    options->fonts[kFont_Status] = fonts_setting.GetFont("status");
  }

  if (!options->fonts[kFont_Text].IsOk()) {
    wxFont font = GetGlobalFont(kDefaultFontSize, GetDefaultFontName());
    options->fonts[kFont_Text] = font;
  }

  GetBool(setting_map, SWITCH_CWD, &options->switch_cwd);

  GetWxString(setting_map, THEME, &options->theme);

  GetBool(setting_map, RESTORE_FILES, &options->restore_files);
  GetBool(setting_map, SHOW_FULL_PATH, &options->show_full_path);
}

// NOTE: If a setting is not provided, don't set the related option.
void ParseEditorOptions(const Setting& setting, editor::Options* options) {
  using namespace editor;

  SettingMap setting_map;
  setting.AsMap(&setting_map);

  //----------------------------------------------------------------------------
  // Text options

  GetInt(setting_map, SHIFT_WIDTH, &options->shift_width);
  GetInt(setting_map, TAB_STOP, &options->tab_stop);
  GetBool(setting_map, EXPAND_TAB, &options->expand_tab);

  GetWString(setting_map, DELIMITERS, &options->delimiters);

  // Indent keys
  Setting ik_setting = GetSetting(setting_map, INDENT_KEYS, Setting::kArray);
  if (ik_setting) {
    options->indent_keys.clear();  // Clear global setting.

    for (int i = 0; i < ik_setting.size(); ++i) {
      const char* str = ik_setting[i].GetString();
      // Assume that the string is pure ascii.
      options->indent_keys.push_back(std::wstring(str, str + strlen(str)));
    }
  }

  // Extra indent options
  GetOptionTable(setting_map["indent"], &options->indent_options);

  // Comment options
  Setting comment_setting = setting_map["comment"];
  if (comment_setting) {
    options->comment_add_space = comment_setting.GetBool("add_space");
    options->comment_respect_indent = comment_setting.GetBool("respect_indent");
  }

  //----------------------------------------------------------------------------
  // View options

  GetInt(setting_map, LINE_PADDING, &options->line_padding);

  GetBool(setting_map, WRAP, &options->wrap);
  GetBool(setting_map, SHOW_NUMBER, &options->show_number);
  GetBool(setting_map, SHOW_SPACE, &options->show_space);
  GetBool(setting_map, SHOW_HSCROLLBAR, &options->show_hscrollbar);

  Setting rulers_setting = GetSetting(setting_map, RULERS, Setting::kArray);
  if (rulers_setting) {
    options->rulers.clear();  // Clear global setting.
    for (int i = 0; i < rulers_setting.size(); ++i) {
      options->rulers.push_back(rulers_setting[i].GetInt());
    }
  }
}

}  // namespace jil
