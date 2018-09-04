#ifndef EDITOR_LINE_NR_AREA_H_
#define EDITOR_LINE_NR_AREA_H_

// Line number area displays the line numbers, fold marks, etc.

#include "wx/panel.h"

namespace editor {

class TextWindow;

class LineNrArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  explicit LineNrArea(TextWindow* text_window);

  virtual ~LineNrArea();

  bool Create(wxWindow* parent, wxWindowID id);

  // Override to accept no focus.
  // Text area should always be focused.
  bool AcceptsFocus() const override {
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

#endif  // EDITOR_LINE_NR_AREA_H_
