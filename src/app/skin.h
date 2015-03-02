#ifndef JIL_SKIN_H_
#define JIL_SKIN_H_
#pragma once

#include "wx/bitmap.h"
#include "wx/string.h"

namespace jil {
namespace skin {

// Note: The bitmap returned will be invalid if it doesn't exist.
wxBitmap GetIcon(const wxString& icon_name);

}  // namespace skin
}  // namespace jil

#endif  // JIL_SKIN_H_
