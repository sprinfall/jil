#include "editor/indent_cpp.h"
#include "editor/indent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

static bool IsMacroHead(const TextLine* line) {
  Coord off = kInvCoord;
  if (line->StartWith(L'#', true, &off) && !line->IsCommentOrString(off)) {
    return true;
  }
  return false;
}

static bool IsMacroBody(const TextBuffer* buffer, Coord ln, Coord* head_ln) {
  for (Coord prev_ln = ln - 1; prev_ln > 0; --prev_ln) {
    const TextLine* prev_line = buffer->Line(prev_ln);
    if (!prev_line->IsEolEscaped(true)) {
      break;
    }
    if (IsMacroHead(prev_line)) {
      *head_ln = prev_ln;
      return true;
    } else {
      continue;
    }
  }
  return false;
}

static bool IsMacro(const TextBuffer* buffer, Coord ln) {
  for (; ln > 0; --ln) {
    if (IsMacroHead(buffer->Line(ln))) {
      return true;
    }

    if (ln > 1) {
      const TextLine* line = buffer->Line(ln - 1);
      if (line->IsEolEscaped(true)) {
        continue;
      }
    }

    break;
  }

  return false;
}

static Coord PrevLine(const TextBuffer* buffer, Coord ln, bool skip_comment, bool skip_macro) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, skip_comment);
  if (skip_macro) {
    while (prev_ln != 0 && IsMacro(buffer, prev_ln)) {
      prev_ln = buffer->PrevNonEmptyLine(prev_ln, skip_comment);
    }
  }
  return prev_ln;
}

static Coord IndentByCurrLine(const TextBuffer* buffer,
                              Coord ln,
                              Coord tab_stop,
                              Coord shift_width) {
  const TextLine* line = buffer->Line(ln);

  // TODO: Check Strings too.
  if (line->CommentsOnly()) {
    return kInvCoord;
  }

  //----------------------------------------------------------------------------

  if (IsMacroHead(line)) {
    return 0;  // No indent for macro definition.
  }

  Coord macro_head_ln = kInvCoord;
  if (IsMacroBody(buffer, ln, &macro_head_ln)) {
    bool indent_macro_body = false;
    buffer->GetIndentOption("indent_macro_body").As<bool>(&indent_macro_body);
    Coord indent = buffer->Line(macro_head_ln)->GetIndent(tab_stop);
    if (indent_macro_body) {
      indent += shift_width;
    }
    return indent;
  }

  //----------------------------------------------------------------------------

  Coord x = kInvCoord;

  if (line->StartWith(L'}', true, &x)) {
    // Find '{'.
    TextPoint p(x, ln);  // '}'
    p = buffer->UnpairedLeftKey(p, L'{', L'}');

    if (!p.Valid()) {
      // Can't find '{', indent the same as previous line.
      Coord prev_ln = PrevLine(buffer, ln, true, true);
      if (prev_ln != 0) {
        return buffer->GetIndent(prev_ln);
      } else {
        return 0;
      }
    }

    // The line with '{'.
    // TODO: Rename temp.
    Coord temp_ln = p.y;
    const TextLine* temp_line = buffer->Line(temp_ln);

    // Check the char before '{'.
    x = temp_line->LastNonSpaceChar(p.x);

    if (x == kInvCoord) {
      // No char before '{'.
      // NOTE: Can't use p.x because there might be tabs.
      return temp_line->GetIndent(tab_stop);
    }

    if (temp_line->Char(x) == L')') {
      // The char before '{' is ')'. (function, if else, switch, etc.)
      p.Set(x, temp_ln);  // ')'
      p = buffer->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != temp_ln) {
        // Find '(' and '(' and ')' are not in the same line.
        // Indent the same as the line with '('.
        return buffer->Line(p.y)->GetIndent(tab_stop);
      }

      // Indent the same as the line with ')'.
      return temp_line->GetIndent(tab_stop);
    }

    // Indent the same as the line ending with '{'.
    return temp_line->GetIndent(tab_stop);
  }

  //----------------------------------------------------------------------------

  if (line->StartWith(L'{', true)) {
    Coord prev_ln = PrevLine(buffer, ln, true, true);
    if (prev_ln == 0) {
      return 0;
    }

    const TextLine* prev_line = buffer->Line(prev_ln);

    x = prev_line->LastNonSpaceChar();

    // function, if else, switch, etc.
    if (prev_line->Char(x) == L')') {
      TextPoint p(x, prev_ln);  // ')'
      p = buffer->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer->Line(prev_ln);
      }
    }

    return prev_line->GetIndent(tab_stop);
  }

  //----------------------------------------------------------------------------

  if (StartWith(line, L"else", true)) {
    Coord prev_ln = buffer->PrevLine(ln, LineStartWith(L"else", L"if"));
    if (prev_ln != 0) {
      return buffer->GetIndent(prev_ln);
    }
    return 0;
  }

  //----------------------------------------------------------------------------

  if (StartWith(line, L"public", L"protected", L"private", true)) {
    Coord prev_ln = buffer->PrevLine(ln, LineStartWith(L"class", L"struct"));
    if (prev_ln != 0) {
      return buffer->GetIndent(prev_ln);
    }
    return 0;
  }

  //----------------------------------------------------------------------------

  if (StartWith(line, L"case", L"default", true)) {
    Coord indent = 0;
    Coord temp_ln = buffer->PrevLine(ln, LineStartWith(L"switch"));
    if (temp_ln != 0) {
      indent = buffer->Line(temp_ln)->GetIndent(tab_stop);

      bool indent_case = false;
      buffer->GetIndentOption("indent_case").As<bool>(&indent_case);

      if (indent_case) {
        indent += shift_width;
      }
    }
    return indent;
  }

  return kInvCoord;
}

static Coord IndentByPrevLine(const TextBuffer* buffer,
                              Coord ln,
                              Coord tab_stop,
                              Coord shift_width,
                              Coord* out_prev_ln = NULL) {
  Coord prev_ln = PrevLine(buffer, ln, true, true);
  if (prev_ln == 0) {
    return 0;
  }

  if (out_prev_ln != NULL) {
    *out_prev_ln = prev_ln;
  }

  const TextLine* prev_line = buffer->Line(prev_ln);
  Coord x = kInvCoord;

  //----------------------------------------------------------------------------

  if (prev_line->EndWith(L'{', true, true, &x)) {
    Coord j = prev_line->LastNonSpaceChar(x);
    if (j == kInvCoord) {
      // No char before '{', check the previous line.
      prev_ln = buffer->PrevNonEmptyLine(prev_ln, true);
      if (prev_ln == 0) {  // No previous line.
        return prev_line->GetIndent(tab_stop) + shift_width;  // TODO: Return?
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

    bool indent_namespace = false;
    buffer->GetIndentOption("indent_namespace").As<bool>(&indent_namespace);

    if (!indent_namespace) {
      if (prev_line->StartWith(L"namespace", true)) {
        return prev_line->GetIndent(tab_stop);
      }
    }

    return prev_line->GetIndent(tab_stop) + shift_width;
  }

  //----------------------------------------------------------------------------

  // public:, protected:, private:, case label:, etc.
  if (prev_line->EndWith(L':', true, true)) {
    return prev_line->GetIndent(tab_stop) + shift_width;
  }

  //----------------------------------------------------------------------------

  // Handle multi-line if, for and function parameters.
  // E.g.,
  // if (name == "a" &&
  //     name != "b")
  x = prev_line->UnpairedLeftKey(L'(', L')');
  if (x != kInvCoord) {
    // Indent the same as the first non-space char after the (.
    return prev_line->TabbedLength(tab_stop, x + 1);
  }

  //----------------------------------------------------------------------------

  if (prev_line->EndWith(L')', true, true, &x)) {
    const TextLine* temp_line = prev_line;
    TextPoint p = buffer->UnpairedLeftKey(TextPoint(x, prev_ln), L'(', L')');
    if (p.Valid() && p.y != prev_ln) {
      temp_line = buffer->Line(p.y);
    }

    if (StartWith(temp_line, L"if", L"else if", L"while", L"for", true)) {
      return temp_line->GetIndent(tab_stop) + shift_width;
    }
  }

  //----------------------------------------------------------------------------

  if (Match(prev_line, L"else", true, true)) {
    return prev_line->GetIndent(tab_stop) + shift_width;
  }

  //----------------------------------------------------------------------------

  //// The EOL of previous line is escaped.
  //if (prev_line->EndWith(L'\\', true)) {
  //  return prev_line->GetIndent(tab_stop) + shift_width;
  //}

  return kInvCoord;
}

// Determine indent by the line before previous line.
// Handle the cases like this:
// if (a > b)
//     return b;
// else             (prev_prev_line)
//     return a;    (prev_line)
// int i;           (line)
static Coord IndentByPrevPrevLine(const TextBuffer* buffer, Coord prev_ln, Coord tab_stop) {
  Coord prev_prev_ln = buffer->PrevNonEmptyLine(prev_ln, true);

  if (prev_prev_ln == 0) {
    return kInvCoord;
  }

  const TextLine* prev_prev_line = buffer->Line(prev_prev_ln);

  if (StartWith(prev_prev_line, L"if", L"else if", L"while", L"for", true)) {
    if (prev_prev_line->UnpairedLeftKey(L'(', L')') == kInvCoord &&
        !prev_prev_line->EndWith(L'{', true, true)) {
      return prev_prev_line->GetIndent(tab_stop);
    }
  } else if (Match(prev_prev_line, L"else", true, true)) {
    if (!prev_prev_line->EndWith(L'{', true, true)) {
      return prev_prev_line->GetIndent(tab_stop);
    }
  }

  return kInvCoord;
}

// TODO:
//   - Class initialization list
Coord IndentCpp(const TextBuffer* buffer, Coord ln) {
  Coord tab_stop = buffer->text_options().tab_stop;
  Coord shift_width = buffer->text_options().shift_width;

  Coord indent = kInvCoord;
  
  indent = IndentByCurrLine(buffer, ln, tab_stop, shift_width);
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

  assert(prev_ln != 0);
  indent = IndentByPrevPrevLine(buffer, prev_ln, tab_stop);
  if (indent != kInvCoord) {
    return indent;
  }

  return buffer->Line(prev_ln)->GetIndent(tab_stop);
}

}  // namespace editor
}  // namespace jil
