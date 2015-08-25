#include "editor/option.h"

namespace jil {
namespace editor {

wxString OptionValue::ToString() const {
  if (type_ == kBool) {
    bool value = false;
    if (As<bool>(&value)) {
      return value ? wxT("true") : wxT("false");
    }
  } else if (type_ == kInt) {
    int value = 0;
    if (As<int>(&value)) {
      return wxString::Format(wxT("%d"), value);
    }
  } else if (type_ == kString) {
    std::string value;
    if (As<std::string>(&value)) {
      return wxString::FromUTF8(value.c_str());
    }
  }

  return wxEmptyString;
}

}  // namespace editor
}  // namespace jil
