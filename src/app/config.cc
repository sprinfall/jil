#include "app/config.h"
#include "wx/log.h"

namespace jil {

// Setting

Setting Setting::Get(const char* name, int type, bool recreate) {
  Setting child = Get(name);
  if (child && child.type() != type) {
    if (recreate) {
      Remove(name);
    }
    child.set_ref(NULL);
  }
  if (!child && recreate) {
    child = Add(name, type);
  }
  return child;
}

void Setting::AsMap(SettingMap* setting_map) const {
  if (ref_ == NULL) {
    return;
  }

  const int size = this->size();

  for (int i = 0; i < size; ++i) {
    Setting setting = Get(i);
    (*setting_map)[setting.name()] = setting;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool Config::Load(const wxString& filename) {
  FILE* config_file = wxFopen(filename, wxT("r"));
  if (config_file == NULL) {
    wxLogInfo(wxT("Failed to open config file to read: %s"), filename.c_str());
    return false;
  }

  int read_code = config_read(&cfg_, config_file);
  fclose(config_file);

  if (read_code == 0) {
    wxLogDebug(wxT("Failed to read config: L%d, %s."),
               config_error_line(&cfg_),
               wxString::FromUTF8(config_error_text(&cfg_)));
    return false;
  }

  return true;
}

bool Config::Save(const wxString& filename) {
  FILE* config_file = wxFopen(filename, wxT("w"));
  if (config_file == NULL) {
    wxLogInfo(wxT("Failed to open config file to write: %s"), filename.c_str());
    return false;
  }

  config_write(&cfg_, config_file);
  fclose(config_file);
  return true;
}

////////////////////////////////////////////////////////////////////////////////

// TODO: Different platform has different font description.

wxFont GetFont(Setting setting) {
  wxFont font;
  font.SetNativeFontInfoUserDesc(wxString::FromUTF8(setting.GetString()));
  return font;
}

wxFont GetFont(Setting parent, const char* name) {
  wxFont font;
  font.SetNativeFontInfoUserDesc(wxString::FromUTF8(parent.GetString(name)));
  return font;
}

void SetFont(Setting parent, const char* name, const wxFont& font) {
  parent.SetString(name, font.GetNativeFontInfoUserDesc().ToUTF8().data());
}

} // namespace jil
