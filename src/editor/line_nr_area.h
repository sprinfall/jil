#ifndef JIL_EDITOR_LINE_NR_AREA_H_
#define JIL_EDITOR_LINE_NR_AREA_H_
#pragma once

// Line number area displays the line numbers, fold marks, etc.

#include "wx/panel.h"

namespace jil {
namespace editor {

class TextWindow;

class LineNrArea : public wxPanel {
  DECLARE_EVENT_TABLE()

 public:
  explicit LineNrArea(TextWindow* text_window);
  bool Create(wxWindow* parent, wxWindowID id);

  // Override to accept no focus.
  // Text area should always be focused.
  virtual bool AcceptsFocus() const override {
    return false;
  }

 protected:
  void OnPaint(wxPaintEvent& evt);
  void OnSize(wxSizeEvent& evt);
  void OnMouse(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

 private:
  TextWindow* text_window_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_LINE_NR_AREA_H_
