#ifndef JIL_SAVE_H_
#define JIL_SAVE_H_
#pragma once

#include <vector>

class wxWindow;

namespace jil {

namespace editor {
class TextBuffer;
}

class TextPage;

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent);
bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent);

// Confirm to save the modified text page.
// Return wxYES, wxNO or wxCANCEL.
int ConfirmSave(TextPage* text_page);

// Save the modified text pages with confirm message.
// Return false if the save is canceled or failed.
bool SaveModifiedWithConfirm(wxWindow* parent,
                             const std::vector<TextPage*>& text_pages);

// Save the new created text pages with confirm message.
// Return false if the save is canceled or failed.
bool SaveNewCreatedWithConfirm(wxWindow* parent,
                               const std::vector<TextPage*>& text_pages);

// Save the given buffer to file.
// If the buffer is new created or read-only, it will be saved as another file.
// Return false on error or canceled (canceled in Save As dialog).
bool Save(editor::TextBuffer* buffer, wxWindow* parent);

}  // namespace jil

#endif  // JIL_SAVE_H_
