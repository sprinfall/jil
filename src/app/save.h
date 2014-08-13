#ifndef JIL_SAVE_H_
#define JIL_SAVE_H_
#pragma once

class wxWindow;

namespace jil {

namespace editor {
class TextBuffer;
}

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent);
bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent);

}  // namespace jil

#endif  // JIL_SAVE_H_
