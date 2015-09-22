#ifndef JIL_EDITOR_FILE_IO_H_
#define JIL_EDITOR_FILE_IO_H_
#pragma once

class wxString;
#include <string>

namespace jil {
namespace editor {

// Save file in bytes.
// Return 0 on success, 1 on IO error.
int SaveBytes(const wxString& filename, const char* bom, const std::string& bytes);

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_FILE_IO_H_
