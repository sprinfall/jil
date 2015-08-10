#ifndef JIL_UI_LABEL_CTRL_H_
#define JIL_UI_LABEL_CTRL_H_
#pragma once

#include "wx/control.h"

namespace jil {
namespace ui {

class LabelCtrl : public wxControl {
  DECLARE_EVENT_TABLE()

public:
  LabelCtrl(wxWindow* parent, wxWindowID id, const wxString& label);
  virtual ~LabelCtrl();

protected:
  virtual wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_LABEL_CTRL_H_
