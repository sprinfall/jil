#ifndef JIL_EDITOR_THEME_H_
#define JIL_EDITOR_THEME_H_
#pragma once

#include <vector>
#include "boost/shared_ptr.hpp"
#include "wx/colour.h"
#include "wx/bitmap.h"

namespace jil {
namespace editor {

class Theme;
typedef boost::shared_ptr<Theme> SharedTheme;

class Theme {
public:
  explicit Theme(size_t theme_size)
      : themes_(theme_size) {
  }

  Theme(size_t theme_size, size_t color_size, size_t image_size)
      : themes_(theme_size), colors_(color_size), images_(image_size) {
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
    if (color.IsOk()) {  // TODO: No check?
      colors_[id] = color;
    }
  }

  const wxBitmap& GetImage(int id) const {
    assert(id >= 0 && id < image_size());
    return images_[id];
  }

  void SetImage(int id, const wxBitmap& image) {
    assert(id >= 0 && id < image_size());
    images_[id] = image;
  }


private:
  int theme_size() const {
    return static_cast<int>(themes_.size());
  }

  int color_size() const {
    return static_cast<int>(colors_.size());
  }

  int image_size() const {
    return static_cast<int>(images_.size());
  }

private:
  std::vector<SharedTheme> themes_;  // Sub-themes
  std::vector<wxColour> colors_;
  std::vector<wxBitmap> images_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_THEME_H_
