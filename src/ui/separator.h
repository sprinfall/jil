#ifndef UI_SEPARATOR_H_
#define UI_SEPARATOR_H_

// Separator line.
// A replacement of wxStaticLine.
// Usage:
// wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
// vsizer->Add(new Separator(this, 1), 0, wxEXPAND);

#include "wx/panel.h"

namespace ui {

class Separator: public wxPanel {
public:
  explicit Separator(wxWindow* parent, int size = 1, bool horizontal = true)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
      , size_(size)
      , horizontal_(horizontal) {
  }

  virtual ~Separator() {
  }

  void SetColor(const wxColour& color) {
    SetBackgroundColour(color);
  }

  // Don't accept focus.
  bool AcceptsFocus() const override {
    return false;
  }

protected:
  wxSize DoGetBestSize() const override {
    if (horizontal_) {
      return wxSize(-1, size_);
    } else {
      return wxSize(size_, -1);
    }
  }

private:
  int size_;
  bool horizontal_;
};

}  // namespace ui

#endif  // UI_SEPARATOR_H_
