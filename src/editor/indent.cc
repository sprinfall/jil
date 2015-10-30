#include "editor/indent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

bool Match(const TextLine* line,
           const std::wstring& str,
           bool ignore_comments,
           bool ignore_spaces,
           Coord* off) {
  Coord off1 = kInvCoord;
  if (!StartWith(line, str, ignore_spaces, &off1)) {
    return false;
  }

  Coord off2 = kInvCoord;
  if (!EndWith(line, str, ignore_comments, ignore_spaces, &off2)) {
    return false;
  }

  if (off1 != off2) {
    return false;
  }

  if (off != NULL) {
    *off = off1;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

Coord IndentCSharp(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentCss(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentCue(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentGo(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  Coord tab_stop = buffer->text_options().tab_stop;
  Coord shift_width = buffer->text_options().shift_width;

  const TextLine* prev_line = buffer->Line(prev_ln);
  const TextLine* line = buffer->Line(ln);

  Coord x = kInvCoord;

  // By default, use the same indent as the previous line.
  return prev_line->GetIndent(tab_stop);
}

Coord IndentHtml(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentJava(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentJavaScript(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentPython(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentRuby(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentTxt(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentVB(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentXml(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

}  // namespace editor
}  // namespace jil
