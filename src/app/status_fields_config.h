#ifndef JIL_STATUS_FIELDS_CONFIG_H_
#define JIL_STATUS_FIELDS_CONFIG_H_
#pragma once

// Parse status bar fields.

#include <vector>
#include "editor/status_bar.h"
#include "app/config.h"

class wxString;

namespace jil {

typedef std::vector<editor::StatusBar::FieldInfo> FieldInfos;

bool ParseStatusFields(Setting fields_setting, FieldInfos* field_infos);

}  // namespace jil

#endif  // JIL_STATUS_FIELDS_CONFIG_H_
