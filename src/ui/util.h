#ifndef JIL_UI_UTIL_H_
#define JIL_UI_UTIL_H_
#pragma once

#include "wx/string.h"

class wxDC;

namespace jil {
namespace ui {

extern const wxString kEllipsis;  // ...

size_t TailorLabel(const wxDC& dc, const wxString& label, int max_width);

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_UTIL_H_