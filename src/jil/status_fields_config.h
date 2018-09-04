#ifndef JIL_STATUS_FIELDS_CONFIG_H_
#define JIL_STATUS_FIELDS_CONFIG_H_

// Parse status bar fields.

#include <vector>

#include "jil/config.h"
#include "jil/status_bar.h"

class wxString;

namespace jil {

typedef std::vector<StatusBar::FieldInfo> FieldInfos;

bool ParseStatusFields(Setting fields_setting, FieldInfos* field_infos);

}  // namespace jil

#endif  // JIL_STATUS_FIELDS_CONFIG_H_
