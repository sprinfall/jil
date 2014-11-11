#include "editor/indent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

bool StartWith(const TextLine* line,
               const std::wstring& str1,
               const std::wstring& str2,
               bool ignore_spaces,
               Coord* off = NULL) {
  return line->StartWith(str1, ignore_spaces, off) ||
         line->StartWith(str2, ignore_spaces, off);
}

bool StartWith(const TextLine* line,
               const std::wstring& str1,
               const std::wstring& str2,
               const std::wstring& str3,
               bool ignore_spaces,
               Coord* off = NULL) {
  return line->StartWith(str1, ignore_spaces, off) ||
         line->StartWith(str2, ignore_spaces, off) ||
         line->StartWith(str3, ignore_spaces, off);
}

Coord IndentCfg(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

Coord IndentCpp(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  // Indent options.
  bool indent_namespace = false;
  bool indent_case = false;

  Coord tab_stop = buffer->options().tab_stop;
  Coord shift_width = buffer->options().shift_width;

  const TextLine* prev_line = buffer->Line(prev_ln);
  const TextLine* line = buffer->Line(ln);

  Coord x = kInvalidCoord;

  // If the current line starts with '}', indent the same as the line with the
  // paired '{'.
  if (line->StartWith(L"}", true, &x)) {
    // Find '{'.
    TextPoint p(x, ln);  // '}'
    p = buffer->UnpairedLeftKey(p, L'{', L'}');

    if (!p.Valid()) {
      // Can't find '{', indent the same as previous line.
      return prev_line->GetIndent(tab_stop);
    }

    // The line with '{'.
    prev_ln = p.y;
    prev_line = buffer->Line(prev_ln);

    // Check the char before '{'.
    Coord i = prev_line->LastNonSpaceChar(p.x);
    if (i == kInvalidCoord) {
      // The line contains only '{', indent '}' the same as '{'.
      // NOTE: Can't use p.x, there might be tabs.
      return prev_line->GetIndent(tab_stop);
    }

    // function, if else, switch, etc.
    if (prev_line->Char(i) == L')') {
      // The char before '{' is ')'.
      p.Set(i, prev_ln);  // ')'
      p = buffer->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer->Line(prev_ln);
      }
    }

    return prev_line->GetIndent(tab_stop);
  }

  if (line->StartWith(L"{")) {
    Coord i = prev_line->LastNonSpaceChar();  // Must be valid.

    // function, if else, switch, etc.
    if (prev_line->Char(i) == L')') {
      TextPoint p(i, prev_ln);  // ')'
      p = buffer->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer->Line(prev_ln);
      }
    }

    return prev_line->GetIndent(tab_stop);
  }

  // Class accessors: public, protected, private.
  if (StartWith(line, L"public", L"protected", L"private", true)) {
    prev_ln = buffer->PrevLine(prev_ln + 1, LineStartWith(L"class", L"struct"));
    if (prev_ln != 0) {
      return buffer->GetIndent(prev_ln);
    }
    return 0;
  }

  if (StartWith(line, L"case", L"default", true)) {
    Coord indent = 0;
    Coord temp_ln = buffer->PrevLine(ln, LineStartWith(L"switch"));
    if (temp_ln != 0) {
      indent = buffer->Line(temp_ln)->GetIndent(tab_stop);
      if (indent_case) {
        indent += shift_width;
      }
    }
    return indent;
  }

  if (prev_line->EndWith(L"{", true, &x)) {
    Coord j = prev_line->LastNonSpaceChar(x);
    if (j == kInvalidCoord) {
      // No char before '{', check the previous line.
      prev_ln = buffer->PrevNonEmptyLine(prev_ln, true);
      if (prev_ln == 0) {  // No previous line.
        return (prev_line->GetIndent(tab_stop) + shift_width);  // TODO: Return?
      }
      prev_line = buffer->Line(prev_ln);
      j = prev_line->LastNonSpaceChar();  // j must be valid.
    }

    // The char before '{' is ')', find the line with the paired '('.
    if (prev_line->Char(j) == L')') {
      TextPoint p(j, prev_ln);  // ')'
      p = buffer->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer->Line(p.y);
      }
    }

    if (!indent_namespace) {  // Don't indent namespace.
      if (prev_line->StartWith(L"namespace")) {
        return prev_line->GetIndent(tab_stop);
      }
    }

    return prev_line->GetIndent(tab_stop) + shift_width;
  }

  // public:/protected:/private:, case label:, etc.
  if (prev_line->EndWith(L":")) {
    return prev_line->GetIndent(tab_stop) + shift_width;
  }

  // TODO
  if (prev_line->EndWith(L",", true, &x)) {
    // Parameters?
    TextPoint p(x, prev_ln);  // ','
    p = buffer->UnpairedLeftKey(p, L'(', L')', true);

    if (p.Valid()) {
      // Indent the same as the first parameter.
      return prev_line->TabbedLength(tab_stop, p.x + 1);
    } else {
      // enum, etc.
      return prev_line->GetIndent(tab_stop);
    }
  }

  if (prev_line->EndWith(L")", true, &x)) {
    const TextLine* temp_line = prev_line;
    TextPoint p = buffer->UnpairedLeftKey(TextPoint(x, prev_ln), L'(', L')');
    if (p.Valid() && p.y != prev_ln) {
      temp_line = buffer->Line(p.y);
    }

    if (StartWith(temp_line, L"if", L"while", L"for", true)) {
      return temp_line->GetIndent(tab_stop) + shift_width;
    }
  }

  // By default, use the same indent as the previous line.
  return prev_line->GetIndent(tab_stop);
}

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

  Coord tab_stop = buffer->options().tab_stop;
  Coord shift_width = buffer->options().shift_width;

  const TextLine* prev_line = buffer->Line(prev_ln);
  const TextLine* line = buffer->Line(ln);

  Coord x = kInvalidCoord;

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
