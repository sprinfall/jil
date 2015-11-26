#include "app/status_fields_config.h"
#include <string>

namespace jil {

static StatusBar::FieldId ParseFieldId(const std::string& field_id_str) {
  using namespace editor;

  static const std::string kFieldIds[StatusBar::kField_Count] = {
    "cwd",
    "tab_options",
    "encoding",
    "file_format",
    "file_type",
    "caret",
#if JIL_ENABLE_LEADER_KEY
    "key_stroke",
#endif
    "space",
  };

  for (size_t i = 0; i < StatusBar::kField_Count; ++i) {
    if (field_id_str == kFieldIds[i]) {
      return static_cast<StatusBar::FieldId>(i);
    }
  }

  return StatusBar::kField_Count;
}

static wxAlignment ParseFieldAlign(const std::string& align_str) {
  if (align_str == "left") {
    return wxALIGN_LEFT;
  } else if (align_str == "right") {
    return wxALIGN_RIGHT;
  } else if (align_str == "center") {
    return wxALIGN_CENTER_HORIZONTAL;
  } else {
    return wxALIGN_NOT;
  }
}

static bool ParseFieldSizeType(const std::string& size_type_str,
                               StatusBar::SizeType& size_type) {
  using namespace editor;

  if (size_type_str == "fit") {
    size_type = StatusBar::kFit;
  } else if (size_type_str == "stretch") {
    size_type = StatusBar::kStretch;
  } else if (size_type_str == "fixed") {
    size_type = StatusBar::kFixed;
  } else if (size_type_str == "percentage") {
    size_type = StatusBar::kPercentage;
  } else {
    return false;
  }

  return true;
}

static bool ParseStatusField(Setting field_setting,
                             StatusBar::FieldInfo* field_info) {
  using namespace editor;

  field_info->id = ParseFieldId(field_setting.GetString("id"));
  if (field_info->id == StatusBar::kField_Count) {
    return false;
  }

  field_info->align = ParseFieldAlign(field_setting.GetString("align"));

  if (!ParseFieldSizeType(field_setting.GetString("size_type"),
                          field_info->size_type)) {
    return false;
  }

  field_info->size_value = field_setting.GetInt("size_value");
  if (field_info->size_value < 0) {
    return false;
  }
  if (field_info->size_value == 0) {
    if (field_info->size_value == StatusBar::kFixed ||
        field_info->size_value == StatusBar::kPercentage) {
      return false;
    }
  }

  field_info->min_size = field_setting.GetInt("min_size");

  return true;
}

bool ParseStatusFields(Setting fields_setting, FieldInfos* field_infos) {
  StatusBar::FieldInfo field_info;

  const int length = fields_setting.size();

  for (int i = 0; i < length; ++i) {
    Setting field_setting = fields_setting.Get(i);
    if (ParseStatusField(field_setting, &field_info)) {
      field_infos->push_back(field_info);
    }
  }

  return true;
}

}  // namespace jil
