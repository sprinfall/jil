#include "app/skin.h"
#include "wx/stdpaths.h"
#include "wx/filename.h"
#include "app/util.h"

namespace jil {
namespace skin {

static wxString GetIconDir() {
  static wxString icon_dir;
  if (icon_dir.empty()) {
    icon_dir = path::ResourceDir() + wxT("icon") + wxFILE_SEP_PATH;
  }
  return icon_dir;
}

wxBitmap GetIcon(const wxString& icon_name) {
  wxBitmap icon(GetIconDir() + icon_name + wxT(".png"), wxBITMAP_TYPE_PNG);
  return icon;
}

}  // namespace skin
}  // namespace jil
