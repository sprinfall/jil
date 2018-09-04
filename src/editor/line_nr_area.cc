#include "editor/line_nr_area.h"
#include "wx/dcbuffer.h"
#include "editor/compile_config.h"
#include "editor/text_window.h"

namespace editor {

BEGIN_EVENT_TABLE(LineNrArea, wxPanel)
EVT_PAINT(LineNrArea::OnPaint)
EVT_SIZE(LineNrArea::OnSize)
EVT_MOUSE_EVENTS(LineNrArea::OnMouse)
EVT_MOUSE_CAPTURE_LOST(LineNrArea::OnMouseCaptureLost)
END_EVENT_TABLE()

LineNrArea::LineNrArea(TextWindow* text_window)
    : text_window_(text_window) {
}

LineNrArea::~LineNrArea() {
}

bool LineNrArea::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  return true;
}

void LineNrArea::OnPaint(wxPaintEvent& WXUNUSED(evt)) {
  wxAutoBufferedPaintDC dc(this);

#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  text_window_->PrepareDC(dc);
  text_window_->OnLineNrPaint(dc);
}

void LineNrArea::OnSize(wxSizeEvent& evt) {
  text_window_->OnLineNrSize(evt);
  evt.Skip();
}

void LineNrArea::OnMouse(wxMouseEvent& evt) {
  bool handled = text_window_->OnLineNrMouse(evt);
  if (!handled) {
    // Skip the event for default handling.
    evt.Skip();
  }
}

void LineNrArea::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  WXUNUSED(evt);
  // Just ignore this event.
}

}  // namespace editor
