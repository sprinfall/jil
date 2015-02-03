#include "app/status_fields_config.h"
#include <string>

namespace jil {

namespace {

editor::StatusBar::FieldId ParseFieldId(const std::string& field_id_str) {
  using namespace editor;

  static const std::string kFieldIds[StatusBar::kField_Count] = {
    "cwd",
    "encoding",
    "file_format",
    "file_type",
    "caret",
    "key_stroke",
    "space",
  };

  for (size_t i = 0; i < StatusBar::kField_Count; ++i) {
    if (field_id_str == kFieldIds[i]) {
      return static_cast<StatusBar::FieldId>(i);
    }
  }

  return StatusBar::kField_Count;
}

wxAlignment ParseFieldAlign(const std::string& align_str) {
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

bool ParseFieldSizeType(const std::string& size_type_str,
                        editor::StatusBar::SizeType& size_type) {
  using namespace editor;

  if (size_type_str == "fit") {
    size_type = StatusBar::kFit;
  } else if (size_type_str == "stretch") {
    size_type = StatusBar::kStretch;
  } else if (size_type_str == "fixed_pixel") {
    size_type = StatusBar::kFixedPixel;
  } else if (size_type_str == "fixed_percentage") {
    size_type = StatusBar::kFixedPercentage;
  } else {
    return false;
  }

  return true;
}

bool ParseStatusField(Setting field_setting,
                      editor::StatusBar::FieldInfo* field_info) {
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

  field_info->size_value = field_setting.GetInt("size");
  if (field_info->size_value < 0) {
    return false;
  }
  if (field_info->size_value == 0) {
    if (field_info->size_value == StatusBar::kFixedPixel ||
        field_info->size_value == StatusBar::kFixedPercentage) {
      return false;
    }
  }

  return true;
}

}  // namespace

bool ParseStatusFields(Setting fields_setting, FieldInfos* field_infos) {
  editor::StatusBar::FieldInfo field_info;

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
