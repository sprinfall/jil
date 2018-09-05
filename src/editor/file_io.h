#ifndef EDITOR_FILE_IO_H_
#define EDITOR_FILE_IO_H_

#include <string>

#include "wx/string.h"

namespace editor {

// Read file in bytes.
// Return 0 on success, 1 on IO error and 2 on exceeding bytes limit.
int ReadBytes(const wxString& filename,
              std::string* bytes,
              std::size_t max_bytes = std::string::npos);

// Save file in bytes.
// The bytes could be empty.
// Return 0 on success, 1 on IO error.
int SaveBytes(const wxString& filename, const char* bom,
              const std::string& bytes);

}  // namespace editor

#endif  // EDITOR_FILE_IO_H_
