#ifndef JIL_SAVE_H_
#define JIL_SAVE_H_
#pragma once

class wxWindow;

namespace jil {

namespace editor {
class TextBuffer;
}

class TextPage;

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent);
bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent);

// Comfirm to save the modified text page.
// Return wxYES, wxNO or wxCANCEL.
int ConfirmSave(TextPage* text_page);

bool Save(editor::TextBuffer* buffer, wxWindow* parent);

}  // namespace jil

#endif  // JIL_SAVE_H_
