#include "editor/style.h"
#include "editor/util.h"

namespace editor {

Style::Style() : values_(kItemCount) {
  // Initialize the major lex styles.
  for (int m = 0; m < kLexMajorTypeCount; ++m) {
    Set(Lex(m));
  }
}

Style::~Style() {
  ClearContainer(&values_);

  for (size_t i = 0; i < kLexMajorTypeCount; ++i) {
    ClearContainer(&lex_values_[i]);
  }
}

void Style::Set(int item, const wxColour& fg, const wxColour& bg, int font) {
  if (values_[item] == NULL) {
    values_[item] = new StyleValue(fg, bg, font);
  } else {
    values_[item]->Set(fg, bg, font);
  }
}

void Style::Set(Lex lex, const wxColour& fg, const wxColour& bg, int font) {
  Values& values = lex_values_[lex.major()];
  if (static_cast<size_t>(lex.minor()) >= values.size()) {
    values.resize(lex.minor() + 1);
  }
  if (values[lex.minor()] == NULL) {
    values[lex.minor()] = new StyleValue(fg, bg, font);
  } else {
    values[lex.minor()]->Set(fg, bg, font);
  }
}

const StyleValue* Style::Get(Lex lex) const {
  const Values& values = lex_values_[lex.major()];
  if (values.empty()) {
    return NULL;
  }

  if (static_cast<size_t>(lex.minor()) < values.size() &&
      values[lex.minor()] != NULL) {
    return values[lex.minor()];
  }

  return values[0];
}

}  // namespace editor
