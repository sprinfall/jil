#ifndef JIL_SEPARATOR_H_
#define JIL_SEPARATOR_H_

// Separator line.
// A replacement of wxStaticLine.
// Usage:
// wxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
// vsizer->Add(new Separator(this, 1, wxHORIZONTAL), 0, wxEXPAND);

#include "wx/control.h"

namespace jil {

// NOTE: If derive from wxPanel there will be focus issue.
class Separator : public wxControl {
public:
  Separator(wxWindow* parent,
            int size = 1,
            wxOrientation orientation = wxHORIZONTAL)
      : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
      , size_(size)
      , orientation_(orientation) {
    SetColor(*wxLIGHT_GREY);
  }

  void SetColor(const wxColour& color) {
    SetBackgroundColour(color);
  }

  virtual ~Separator() {
  }

  // Don't accept focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

protected:
  virtual wxSize DoGetBestSize() const override {
    if (orientation_ == wxHORIZONTAL) {
      return wxSize(-1, size_);
    } else {
      return wxSize(size_, -1);
    }
  }

private:
  int size_;
  wxOrientation orientation_;
};

}  // namespace jil

#endif  // JIL_SEPARATOR_H_
