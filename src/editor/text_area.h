#ifndef EDITOR_TEXT_AREA_H_
#define EDITOR_TEXT_AREA_H_

#include "wx/panel.h"

namespace editor {

class TextWindow;

class TextArea : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  explicit TextArea(TextWindow* text_window);
  bool Create(wxWindow* parent, wxWindowID id);

  // Override to scroll line nr area together.
  void ScrollWindow(int dx, int dy, const wxRect* rect = NULL) override;

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnSize(wxSizeEvent& evt);
  void OnMouse(wxMouseEvent& evt);
  void OnKeyDown(wxKeyEvent& evt);
  void OnChar(wxKeyEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);

private:
  TextWindow* text_window_;
};

}  // namespace editor

#endif  // EDITOR_TEXT_AREA_H_
