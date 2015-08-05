#ifndef JIL_UI_STATIC_BOX_H_
#define JIL_UI_STATIC_BOX_H_
#pragma once

// A replacement of wxStaticBox.

#include "wx/panel.h"

namespace jil {
namespace ui {

class StaticBox : public wxPanel{
  DECLARE_EVENT_TABLE()

public:
  StaticBox(wxWindow* parent, const wxString& label);
  virtual ~StaticBox();

  void set_border_colour(const wxColour& border_colour) {
    border_colour_ = border_colour;
  }

  // Don't call SetSizer().
  void SetBodySizer(wxSizer* body_sizer);

protected:
  void OnPaint(wxPaintEvent& evt);

private:
  wxSize padding_;
  wxColour border_colour_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_STATIC_BOX_H_
