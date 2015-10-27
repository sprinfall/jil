#include "editor/indent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////
// Helpers

inline
static bool StartWith(const TextLine* line,
                      const std::wstring& str,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return line->StartWith(str, ignore_spaces, off);
}

static bool StartWith(const TextLine* line,
                      const std::wstring& str1,
                      const std::wstring& str2,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return StartWith(line, str1, ignore_spaces, off) ||
         StartWith(line, str2, ignore_spaces, off);
}

static bool StartWith(const TextLine* line,
                      const std::wstring& str1,
                      const std::wstring& str2,
                      const std::wstring& str3,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return StartWith(line, str1, str2, ignore_spaces, off) ||
         StartWith(line, str3, ignore_spaces, off);
}

static bool StartWith(const TextLine* line,
                      const std::wstring& str1,
                      const std::wstring& str2,
                      const std::wstring& str3,
                      const std::wstring& str4,
                      bool ignore_spaces,
                      Coord* off = NULL) {
  return StartWith(line, str1, str2, str3, ignore_spaces, off) ||
         StartWith(line, str4, ignore_spaces, off);
}

inline
static bool EndWith(const TextLine* line,
                    const std::wstring& str,
                    bool ignore_comments,
                    bool ignore_spaces,
                    Coord* off = NULL) {
  return line->EndWith(str, ignore_comments, ignore_spaces, off);
}

static bool Match(const TextLine* line,
                  const std::wstring& str,
                  bool ignore_comments,
                  bool ignore_spaces,
                  Coord* off = NULL) {
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

static Coord GetPrevLineIndent(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }
  return buffer->GetIndent(prev_ln);
}

static bool IsLexCommentOrString(Lex lex) {
  if (lex.major() == kLexComment) {
    return true;
  }
  if (lex == Lex(kLexConstant, kLexConstantString)) {
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

Coord IndentCfg(const TextBuffer* buffer, Coord ln) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

namespace cpp {

bool IsEolEscaped(TextLine* line) {

}

bool IsLineMacro(const TextBuffer* buffer, Coord ln) {
  for (; ln > 0; --ln) {
    TextLine* line = buffer->Line(ln);

    Coord off = kInvCoord;
    if (line->StartWith(L'#', true, &off) &&
        !IsLexCommentOrString(line->GetLex(off))) {
      return true;
    }

    // TODO: \ should be unescaped
    if (ln > 1 && buffer->Line(ln - 1)->EndWith(L'\\', false, false)) {
      continue;
    }

    break;
  }

  return false;
}

Coord PrevNonEmptyLine(const TextBuffer* buffer,
                       Coord ln,
                       bool skip_comment,
                       bool skip_macro) {
  Coord prev_ln = buffer->PrevNonEmptyLine(ln, skip_comment);
  if (skip_macro) {
    while (prev_ln != 0 && IsLineMacro(buffer, prev_ln)) {
      prev_ln = buffer->PrevNonEmptyLine(ln, skip_comment);
    }
  }
  return prev_ln;
}

// Determine indent by current line.
// Return kInvCoord if failed to determine it.
Coord IndentByCurrLine(const TextBuffer* buffer,
                       Coord ln,
                       Coord tab_stop,
                       Coord shift_width) {
  const TextLine* line = buffer->Line(ln);

  // TODO: Check Strings too.
  if (line->CommentsOnly()) {
    return kInvCoord;
  }

  //----------------------------------------------------------------------------

  if (line->StartWith(L'#', true)) {
    return 0;  // No indent for macro definition.
  }

  if (IsLineMacro(line)) {
  }

  Coord x = kInvCoord;

  //----------------------------------------------------------------------------

  if (line->StartWith(L'}', true, &x)) {
    // If the current line starts with '}', indent the same as the line with
    // the paired '{'.

    // Find '{'.
    TextPoint p(x, ln);  // '}'
    p = buffer->UnpairedLeftKey(p, L'{', L'}');

    if (!p.Valid()) {
      // Can't find '{', indent the same as previous line.
      return GetPrevLineIndent(buffer, ln);
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
    Coord prev_ln = buffer->PrevNonEmptyLine(ln, true);
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

// Determine indent by previous line.
// Return kInvCoord if failed to determine it.
Coord IndentByPrevLine(const TextBuffer* buffer,
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

  //----------------------------------------------------------------------------

  if (prev_line->EndWith(L'{', true, true, &x)) {
    Coord j = prev_line->LastNonSpaceChar(x);
    if (j == kInvCoord) {
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

  if (prev_line->EndWith(L',', true, true, &x)) {
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
Coord IndentByPrevPrevLine(const TextBuffer* buffer, Coord prev_ln, Coord tab_stop) {
  Coord prev_prev_ln = buffer->PrevNonEmptyLine(prev_ln, true);

  if (prev_prev_ln == 0) {
    return kInvCoord;
  }

  const TextLine* prev_prev_line = buffer->Line(prev_prev_ln);

  if (StartWith(prev_prev_line, L"if", L"else if", L"while", L"for", true)) {
    return prev_prev_line->GetIndent(tab_stop);
  }

  if (Match(prev_prev_line, L"else", true, true)) {
    return prev_prev_line->GetIndent(tab_stop);
  }

  return kInvCoord;
}

}  // namespace cpp


Coord IndentCpp(const TextBuffer* buffer, Coord ln) {
  Coord tab_stop = buffer->text_options().tab_stop;
  Coord shift_width = buffer->text_options().shift_width;

  Coord indent = kInvCoord;
  
  indent = cpp::IndentByCurrLine(buffer, ln, tab_stop, shift_width);
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
  indent = cpp::IndentByPrevLine(buffer, ln, tab_stop, shift_width, &prev_ln);
  if (indent != kInvCoord) {
    return indent;
  }

  assert(prev_ln != 0);
  indent = cpp::IndentByPrevPrevLine(buffer, prev_ln, tab_stop);
  if (indent != kInvCoord) {
    return indent;
  }

  return buffer->Line(prev_ln)->GetIndent(tab_stop);
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
