#ifndef JIL_CONFIG_H_
#define JIL_CONFIG_H_
#pragma once

// C++ wrapper for libconfig C interface.

#include <string>
#include <map>
#include "wx/string.h"
#include "wx/colour.h"
#include "wx/gdicmn.h"
#include "wx/font.h"
#include "libconfig/libconfig.h"

namespace jil {

class Setting;
typedef std::map<std::string, Setting> SettingMap;

// Wrapper of config_setting_t.
class Setting {
 public:
  enum Type {
    // Make type names simpler.
    kNone     = CONFIG_TYPE_NONE,
    kGroup    = CONFIG_TYPE_GROUP,
    kInt      = CONFIG_TYPE_INT,
    kInt64    = CONFIG_TYPE_INT64,
    kFloat    = CONFIG_TYPE_FLOAT,
    kString   = CONFIG_TYPE_STRING,
    kBool     = CONFIG_TYPE_BOOL,
    kArray    = CONFIG_TYPE_ARRAY,
    kList     = CONFIG_TYPE_LIST,
  };

  Setting(config_setting_t* ref = NULL) : ref_(ref) {
  }

  operator bool() const { return ref_ != NULL; }

  const char* name() const {
    return ref_->name;
  }

  int type() const {
    return ref_->type;
  }

  unsigned int line() const {
    return ref_->line;
  }

  const char* file() const {
    return ref_->file;
  }

  // For aggregate setting.

  int size() const {
    return config_setting_length(ref_);
  }

  Setting Get(int index) const {
    return Setting(config_setting_get_elem(ref_, index));
  }

  Setting operator[](int index) const {
    return Get(index);
  }

  Setting Get(const char* name) const {
    return Setting(config_setting_get_member(ref_, name));
  }

  Setting Get(const char* name, int type, bool recreate = false);

  Setting Add(const char* name, int type) {
    return Setting(config_setting_add(ref_, name, type));
  }

  void Remove(const char* name) {
    config_setting_remove(ref_, name);
  }

  // For scalar setting.

  int GetInt() const {
    return config_setting_get_int(ref_);
  }

  void SetInt(int i) {
    config_setting_set_int(ref_, i);
  }

  bool GetBool() const {
    return config_setting_get_bool(ref_) != 0;
  }

  void SetBool(bool b) {
    config_setting_set_bool(ref_, b);
  }

  double GetFloat() const {
    return config_setting_get_float(ref_);
  }

  void SetFloat(double f) {
    config_setting_set_float(ref_, f);
  }

  const char* GetString() const {
    const char* str = config_setting_get_string(ref_);
    return str == NULL ? "" : str;
  }

  void SetString(const char* str) {
    config_setting_set_string(ref_, str);
  }

  wxColour GetColor() const {
    wxColour color;
    color.Set(wxString::FromAscii(GetString()));
    return color;
  }

  wxFont GetFont() const {
    wxFont font;
    // NOTE: Different platforms have different font description.
    font.SetNativeFontInfoUserDesc(wxString::FromUTF8(GetString()));
    return font;
  }

  void SetFont(const wxFont& font) {
    // NOTE: Different platforms have different font description.
    SetString(font.GetNativeFontInfoUserDesc().ToUTF8().data());
  }

  // For group setting.

  int GetInt(const char* name) const {
    int value = 0;
    config_setting_lookup_int(ref_, name, &value);
    return value;
  }

  void SetInt(const char* name, int i) {
    Setting child_setting = Get(name, kInt, true);
    child_setting.SetInt(i);
  }

  bool GetBool(const char* name) const {
    int b = 0;
    config_setting_lookup_bool(ref_, name, &b);
    return b != 0;
  }

  void SetBool(const char* name, bool b) {
    Setting child_setting = Get(name, kBool, true);
    child_setting.SetBool(b);
  }

  double GetFloat(const char* name) const {
    double value = 0.0f;
    config_setting_lookup_float(ref_, name, &value);
    return value;
  }

  void SetFloat(const char* name, double f) {
    Setting child_setting = Get(name, kFloat, true);
    child_setting.SetFloat(f);
  }

  const char* GetString(const char* name) const {
    const char* str = "";
    config_setting_lookup_string(ref_, name, &str);
    return str;
  }

  void SetString(const char* name, const char* str) {
    Setting child_setting = Get(name, kString, true);
    child_setting.SetString(str);
  }

  wxColour GetColor(const char* name) const {
    wxColour color;
    color.Set(wxString::FromAscii(GetString(name)));
    return color;
  }

  wxFont GetFont(const char* name) const {
    wxFont font;
    // NOTE: Different platforms have different font description.
    font.SetNativeFontInfoUserDesc(wxString::FromUTF8(GetString(name)));
    return font;
  }

  void SetFont(const char* name, const wxFont& font) {
    // NOTE: Different platforms have different font description.
    SetString(name, font.GetNativeFontInfoUserDesc().ToUTF8().data());
  }

  void AsMap(SettingMap* setting_map) const;

 private:
  void set_ref(config_setting_t* ref = NULL) {
    ref_ = ref;
  }

  config_setting_t* ref_;  // Reference only.
};

////////////////////////////////////////////////////////////////////////////////

// Wrapper of config_t.
class Config {
 public:
  Config() {
    config_init(&cfg_);
  }

  ~Config() {
    config_destroy(&cfg_);
  }

  bool Load(const wxString& filename);
  bool Save(const wxString& filename);

  Setting Root() const {
    return Setting(cfg_.root);
  }

  Setting Find(const char* path) const {
    return Setting(config_lookup(&cfg_, path));
  }

 private:
  config_t cfg_;
};

}  // namespace jil

#endif  // JIL_CONFIG_H_
