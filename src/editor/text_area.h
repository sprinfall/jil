#ifndef JIL_EDITOR_TEXT_AREA_H_
#define JIL_EDITOR_TEXT_AREA_H_
#pragma once

#include "wx/panel.h"

namespace jil {
namespace editor {

class TextWindow;

class TextArea : public wxPanel {
  DECLARE_EVENT_TABLE()

 public:
  explicit TextArea(TextWindow* text_window);
  bool Create(wxWindow* parent, wxWindowID id);

  // Override to scroll line nr area together.
  virtual void ScrollWindow(int dx, int dy, const wxRect* rect = NULL) override;

 protected:
  void OnPaint(wxPaintEvent& evt);
  void OnSize(wxSizeEvent& evt);
  void OnMouse(wxMouseEvent& evt);
  void OnKeyDown(wxKeyEvent& evt);
  void OnChar(wxKeyEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

 private:
  TextWindow* text_window_;
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TEXT_AREA_H_
