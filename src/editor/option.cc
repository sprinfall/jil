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

bool OptionValue::Parse(const wxString& str) {
  if (type_ == kBool) {
    if (str.CmpNoCase(wxT("true")) == 0) {
      data_ = true;
      return true;
    } else if (str.CmpNoCase(wxT("false")) == 0) {
      data_ = false;
      return true;
    }
  } else if (type_ == kInt) {
    long value = 0;
    if (str.ToLong(&value)) {
      data_ = static_cast<int>(value);
      return true;
    }
  } else if (type_ == kString) {
    // NOTE: Don't assign str.ToUTF8().data() directly to data_!!!
    data_ = std::string(str.ToUTF8().data()); 
    return true;
  }

  return false;
}

bool operator==(const ViewOptions& lhs, const ViewOptions& rhs) {
  if (lhs.wrap == rhs.wrap &&
      lhs.show_number == rhs.show_number &&
      lhs.show_space == rhs.show_space &&
      lhs.show_hscrollbar == rhs.show_hscrollbar &&
      lhs.rulers == rhs.rulers) {
    return true;
  }
  return false;
}

}  // namespace editor
}  // namespace jil
