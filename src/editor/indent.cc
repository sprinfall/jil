#include "editor/indent.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

bool StartWith(const TextLine* line,
               const std::wstring& str1,
               const std::wstring& str2) {
  return line->StartWith(str1) || line->StartWith(str2);
}

bool StartWith(const TextLine* line,
               const std::wstring& str1,
               const std::wstring& str2,
               const std::wstring& str3) {
  return line->StartWith(str1) ||
         line->StartWith(str2) ||
         line->StartWith(str3);
}

////////////////////////////////////////////////////////////////////////////////

void IndentFunc::SetBuffer(const TextBuffer* buffer) {
  buffer_ = buffer;

  tab_stop_ = buffer_->options().tab_stop;
  shift_width_ = buffer_->options().shift_width;
}

////////////////////////////////////////////////////////////////////////////////

IndentTxt::IndentTxt() {
}

Coord IndentTxt::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentCpp::IndentCpp()
    : indent_namespace_(false)
    , indent_case_(false) {
}

Coord IndentCpp::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  const TextLine* prev_line = buffer_->Line(prev_ln);
  const TextLine* line = buffer_->Line(ln);

  Coord x = kInvalidCoord;

  // If the current line starts with '}', indent the same as the line with the
  // paired '{'.
  if (line->StartWith(L"}", true, &x)) {
    // Find '{'.
    TextPoint p(x, ln);  // '}'
    p = buffer_->UnpairedLeftKey(p, L'{', L'}');

    if (!p.Valid()) {
      // Can't find '{', indent the same as previous line.
      return prev_line->GetIndent(tab_stop_);
    }

    // The line with '{'.
    prev_ln = p.y;
    prev_line = buffer_->Line(prev_ln);

    // Check the char before '{'.
    Coord i = prev_line->LastNonSpaceChar(p.x);
    if (i == kInvalidCoord) {
      // The line contains only '{', indent '}' the same as '{'.
      // NOTE: Can't use p.x, there might be tabs.
      return prev_line->GetIndent(tab_stop_);
    }

    // function, if else, switch, etc.
    if (prev_line->Char(i) == L')') {
      // The char before '{' is ')'.
      p.Set(i, prev_ln);  // ')'
      p = buffer_->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer_->Line(prev_ln);
      }
    }

    return prev_line->GetIndent(tab_stop_);
  }

  if (line->StartWith(L"{")) {
    Coord i = prev_line->LastNonSpaceChar();  // Must be valid.

    // function, if else, switch, etc.
    if (prev_line->Char(i) == L')') {
      TextPoint p(i, prev_ln);  // ')'
      p = buffer_->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer_->Line(prev_ln);
      }
    }

    return prev_line->GetIndent(tab_stop_);
  }

  // Class accessors: public, protected, private.
  if (StartWith(line, L"public", L"protected", L"private")) {
    prev_ln = buffer_->PrevLine(prev_ln + 1,
                                LineStartWith(L"class", L"struct"));
    if (prev_ln != 0) {
      return buffer_->GetIndent(prev_ln);
    }
    return 0;
  }

  if (StartWith(line, L"case", L"default")) {
    Coord indent = 0;
    Coord temp_ln = buffer_->PrevLine(ln, LineStartWith(L"switch"));
    if (temp_ln != 0) {
      indent = buffer_->Line(temp_ln)->GetIndent(tab_stop_);
      if (indent_case_) {
        indent += shift_width_;
      }
    }
    return indent;
  }

  if (prev_line->EndWith(L"{", true, &x)) {
    Coord j = prev_line->LastNonSpaceChar(x);
    if (j == kInvalidCoord) {
      // No char before '{', check the previous line.
      prev_ln = buffer_->PrevNonEmptyLine(prev_ln);
      if (prev_ln == 0) {  // No previous line.
        return (prev_line->GetIndent(tab_stop_) + shift_width_);  // TODO: Return?
      }
      prev_line = buffer_->Line(prev_ln);
      j = prev_line->LastNonSpaceChar();  // j must be valid.
    }

    // The char before '{' is ')', find the line with the paired '('.
    if (prev_line->Char(j) == L')') {
      TextPoint p(j, prev_ln);  // ')'
      p = buffer_->UnpairedLeftKey(p, L'(', L')');

      if (p.Valid() && p.y != prev_ln) {
        prev_ln = p.y;
        prev_line = buffer_->Line(p.y);
      }
    }

    if (!indent_namespace_) {  // Don't indent namespace.
      if (prev_line->StartWith(L"namespace")) {
        return prev_line->GetIndent(tab_stop_);
      }
    }

    return prev_line->GetIndent(tab_stop_) + shift_width_;
  }

  // public:/protected:/private:, case label:, etc.
  if (prev_line->EndWith(L":")) {
    return prev_line->GetIndent(tab_stop_) + shift_width_;
  }

  // TODO
  if (prev_line->EndWith(L",", true, &x)) {
    // Parameters?
    TextPoint p(x, prev_ln);  // ','
    p = buffer_->UnpairedLeftKey(p, L'(', L')', true);

    if (p.Valid()) {
      // Indent the same as the first parameter.
      return prev_line->TabbedLength(tab_stop_, p.x + 1);
    } else {
      // enum, etc.
      return prev_line->GetIndent(tab_stop_);
    }
  }

  if (prev_line->EndWith(L")", true, &x)) {
    const TextLine* temp_line = prev_line;
    TextPoint p = buffer_->UnpairedLeftKey(TextPoint(x, prev_ln), L'(', L')');
    if (p.Valid() && p.y != prev_ln) {
      temp_line = buffer_->Line(p.y);
    }

    if (temp_line->StartWith(L"if", true) ||
        temp_line->StartWith(L"while", true) ||
        temp_line->StartWith(L"for", true)) {
      return temp_line->GetIndent(tab_stop_) + shift_width_;
    }
  }

  // By default, use the same indent as the previous line.
  return prev_line->GetIndent(tab_stop_);
}

////////////////////////////////////////////////////////////////////////////////

IndentJava::IndentJava() {
}

Coord IndentJava::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentCSharp::IndentCSharp() {
}

Coord IndentCSharp::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentPython::IndentPython() {
}

Coord IndentPython::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentRuby::IndentRuby() {
}

Coord IndentRuby::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentGo::IndentGo() {
}

Coord IndentGo::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  const TextLine* prev_line = buffer_->Line(prev_ln);
  const TextLine* line = buffer_->Line(ln);

  Coord x = kInvalidCoord;

  // By default, use the same indent as the previous line.
  return prev_line->GetIndent(tab_stop_);
}

////////////////////////////////////////////////////////////////////////////////

IndentJavaScript::IndentJavaScript() {
}

Coord IndentJavaScript::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentXml::IndentXml() {
}

Coord IndentXml::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentHtml::IndentHtml() {
}

Coord IndentHtml::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentCss::IndentCss() {
}

Coord IndentCss::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentCue::IndentCue() {
}

Coord IndentCue::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentCfg::IndentCfg() {
}

Coord IndentCfg::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

////////////////////////////////////////////////////////////////////////////////

IndentVB::IndentVB() {
}

Coord IndentVB::Indent(Coord ln) {
  Coord prev_ln = buffer_->PrevNonEmptyLine(ln);
  if (prev_ln == 0) {
    return 0;
  }

  return buffer_->GetIndent(prev_ln);
}

}  // namespace editor
}  // namespace jil
