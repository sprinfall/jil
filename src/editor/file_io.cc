#include "editor/file_io.h"

#include "wx/defs.h"
#include "wx/wxcrt.h"

namespace editor {

int ReadBytes(const wxString& filename, std::string* bytes, size_t max_bytes) {
  // If we don't use binary mode to read text files, line ending characters
  // might be "lost". Suppose there's a file with the following text:
  // abc\r\nde\r\nf
  // The file size told by ftell() is 10, but fread() can only read 8.
  // So we use binary mode.
  FILE* file = wxFopen(filename, _T("rb"));
  if (file == NULL) {
    return 1;
  }

  fseek(file, 0, SEEK_END);
  long fsize = ftell(file);
  if (fsize == -1) {
    fclose(file);
    return 1;
  }
  if (fsize == 0) {
    fclose(file);
    return 0;
  }

  rewind(file);

  int result = 0;

  size_t count = static_cast<size_t>(fsize);

  if (max_bytes != std::string::npos) {
    // Limit bytes.
    if (count > max_bytes) {
      count = max_bytes;
      result = 2;
    }
  }

  bytes->resize(count);
  size_t read_size = fread(&(*bytes)[0], 1, count, file);
  if (read_size != count) {
    result = 1;
  }

  fclose(file);

  return result;
}

int SaveBytes(const wxString& filename, const char* bom,
              const std::string& bytes) {
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
