#include "editor/indent_cfg.h"
#include "editor/indent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

static Coord IndentByCurrLine(const TextBuffer* buffer, Coord ln) {
  const TextLine* line = buffer->Line(ln);

  if (line->CommentsOnly()) {
    return kInvCoord;
  }

  Coord x = kInvCoord;

  // If the current line starts with '}', indent the same as the line with
  // the paired '{'. The same applies to ')' and ']'.

  const std::pair<wchar_t, wchar_t> kBrackets[] = {
    std::make_pair(L'{', L'}'),
    std::make_pair(L'(', L')'),
    std::make_pair(L'[', L']'),
  };

  for (size_t i = 0; i < 3; ++i) {
    const std::pair<wchar_t, wchar_t>& bracket_pair = kBrackets[i];

    if (line->StartWith(bracket_pair.second, true, &x)) {
      TextPoint p(x, ln);
      p = buffer->UnpairedLeftKey(p, bracket_pair.first, bracket_pair.second);
      if (p.Valid()) {
        return buffer->GetIndent(p.y);
      } else {
        return buffer->GetPrevNonEmptyLineIndent(ln, true);
      }
    }
  }

  return kInvCoord;
}

static Coord IndentByPrevLine(const TextBuffer* buffer,
                              Coord ln,
                              Coord tab_stop,
                              Coord shift_width,
                              Coord* out_prev_ln = NULL) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  if (out_prev_ln != NULL) {
    *out_prev_ln = prev_ln;
  }

  const TextLine* prev_line = buffer->Line(prev_ln);
  Coord x = kInvCoord;

  if (EndWith(prev_line, L"{", L"(", L"[", true, true)) {
    return prev_line->GetIndent(tab_stop) + shift_width;
  }

  return kInvCoord;
}

Coord IndentCfg(const TextBuffer* buffer, Coord ln) {
  Coord tab_stop = buffer->text_options().tab_stop;
  Coord shift_width = buffer->text_options().shift_width;

  Coord indent = kInvCoord;

  indent = IndentByCurrLine(buffer, ln);
  if (indent != kInvCoord) {
    return indent;
  }

  TextLine* line = buffer->Line(ln);

  if (line->CommentsOnly()) {
    Coord prev_ln = buffer->PrevNonEmptyLine(ln, false);
    if (prev_ln != 0) {
      TextLine* prev_line = buffer->Line(prev_ln);
      if (prev_line->CommentsOnly()) {
        return prev_line->GetIndent(tab_stop);
      }
    }
  }

  Coord prev_ln = kInvCoord;
  indent = IndentByPrevLine(buffer, ln, tab_stop, shift_width, &prev_ln);
  if (indent != kInvCoord) {
    return indent;
  }

  return buffer->Line(prev_ln)->GetIndent(tab_stop);
}

}  // namespace editor
}  // namespace jil
