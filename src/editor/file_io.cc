#include "editor/file_io.h"
#include "wx/defs.h"
#include "wx/wxcrt.h"

namespace jil {
namespace editor {

int SaveBytes(const wxString& filename, const char* bom, const std::string& bytes) {
  FILE* file = wxFopen(filename, wxT("wb"));
  if (file == NULL) {
    return 1;
  }

  int result = 0;

  if (bom != NULL) {
    size_t bom_len = strlen(bom);
    if (fwrite(bom, 1, bom_len, file) != bom_len) {
      fclose(file);
      return 1;
    }
  }

  size_t write_size = fwrite(bytes.c_str(), 1, bytes.size(), file);
  if (write_size != bytes.size()) {
    result = 1;
  }

  fclose(file);
  return result;
}

}  // namespace editor
}  // namespace jil
