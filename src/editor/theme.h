#ifndef JIL_EDITOR_THEME_H_
#define JIL_EDITOR_THEME_H_
#pragma once

#include <vector>
#include "boost/shared_ptr.hpp"
#include "wx/colour.h"
#include "wx/font.h"

namespace jil {
namespace editor {

class Theme;
typedef boost::shared_ptr<Theme> SharedTheme;

class Theme {
public:
  explicit Theme(size_t theme_size)
      : themes_(theme_size) {
  }

  Theme(size_t color_size, size_t font_size)
      : colors_(color_size), fonts_(font_size) {
  }

  Theme(size_t theme_size, size_t color_size, size_t font_size)
      : themes_(theme_size), colors_(color_size), fonts_(font_size) {
  }

  SharedTheme GetTheme(int id) const {
    assert(id >= 0 && id < theme_size());
    return themes_[id];
  }

  void SetTheme(int id, const SharedTheme& theme) {
    assert(id >= 0 && id < theme_size());
    themes_[id] = theme;
  }

  const wxColour& GetColor(int id) const {
    assert(id >= 0 && id < color_size());
    return colors_[id];
  }

  void SetColor(int id, const wxColour& color) {
    assert(id >= 0 && id < color_size());
    if (color.IsOk()) {
      colors_[id] = color;
    }
  }

  const wxFont& GetFont(int id) const {
    assert(id >= 0 && id < font_size());
    return fonts_[id];
  }

  void SetFont(int id, const wxFont& font) {
    assert(id >= 0 && id < font_size());
    if (font.IsOk()) {
      fonts_[id] = font;
    }
  }

private:
  int theme_size() const { return static_cast<int>(themes_.size()); }
  int color_size() const { return static_cast<int>(colors_.size()); }
  int font_size() const { return static_cast<int>(fonts_.size()); }

private:
  std::vector<SharedTheme> themes_;  // Sub-themes
  std::vector<wxColour> colors_;
  std::vector<wxFont> fonts_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_THEME_H_
