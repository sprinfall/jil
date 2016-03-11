#include "app/defs.h"
#include "app/i18n_strings.h"

namespace jil {
  
const wxString kInternalFtIdPrefix = wxT("jil-");

// Internal file type IDs.
// Format: "jil-xxx_yyy_zzz"
const wxString kFindResultFtId = wxT("jil-find_result");

const wxString kTxtFtId = wxT("txt");

const editor::FileType kTxtFt(kTxtFtId, kTrPlainText);

}  // namespace jil

