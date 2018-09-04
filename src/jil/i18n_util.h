#ifndef JIL_I18N_UTIL_H_
#define JIL_I18N_UTIL_H_

#include "wx/intl.h"
#include "wx/string.h"

#define kTrColon _(":")

namespace jil {

// Append a translated colon to the given string.
inline wxString TrColon(const wxChar* tr_str) {
  return wxString(tr_str) + kTrColon;
}

}  // namespace jil

#endif  // JIL_I18N_UTIL_H_
