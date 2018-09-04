#include "editor/text_area.h"
#include "wx/dcbuffer.h"
#include "wx/log.h"
#include "editor/text_window.h"
#include "editor/line_nr_area.h"
#include "editor/renderer.h"

#define kTextAreaStyle (wxBORDER_NONE | wxWANTS_CHARS)

namespace editor {

BEGIN_EVENT_TABLE(TextArea, wxPanel)
EVT_PAINT               (TextArea::OnPaint)
EVT_SIZE                (TextArea::OnSize)
EVT_MOUSE_EVENTS        (TextArea::OnMouse)
EVT_KEY_DOWN            (TextArea::OnKeyDown)
EVT_CHAR                (TextArea::OnChar)
EVT_MOUSE_CAPTURE_LOST  (TextArea::OnMouseCaptureLost)
EVT_SET_FOCUS           (TextArea::OnSetFocus)
EVT_KILL_FOCUS          (TextArea::OnKillFocus)
END_EVENT_TABLE()

TextArea::TextArea(TextWindow* text_window)
    : text_window_(text_window) {
}

bool TextArea::Create(wxWindow* parent, wxWindowID id) {
  if (!wxPanel::Create(parent,
                       id,
                       wxDefaultPosition,
                       wxDefaultSize,
                       kTextAreaStyle)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);
  return true;
}

void TextArea::ScrollWindow(int dx, int dy, const wxRect* rect) {
  wxPanel::ScrollWindow(dx, dy, rect);

  if (dy != 0) {
    text_window_->line_nr_area()->ScrollWindow(0, dy, NULL);
  }
}

void TextArea::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  //wxLogDebug("TextArea native double buffer");
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  text_window_->PrepareDC(dc);
  Renderer renderer(&dc);
  text_window_->OnTextPaint(renderer);
}

void TextArea::OnSize(wxSizeEvent& evt) {
  text_window_->OnTextSize(evt);
  evt.Skip();
}

void TextArea::OnMouse(wxMouseEvent& evt) {
  bool handled = text_window_->OnTextMouse(evt);
  if (!handled) {
    // Skip the event for default handling.
    evt.Skip();
  }
}

void TextArea::OnKeyDown(wxKeyEvent& evt) {
  bool handled = text_window_->OnTextKeyDown(evt);
  if (!handled) {
    // Skip the event for default handling.
    evt.Skip();
  }
}

void TextArea::OnChar(wxKeyEvent& evt) {
  text_window_->OnTextChar(evt);
}

void TextArea::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt) {
  text_window_->OnTextMouseCaptureLost(evt);
}

void TextArea::OnSetFocus(wxFocusEvent& evt) {
  text_window_->OnTextSetFocus(evt);
  evt.Skip();  // Skip for default handling.
}

void TextArea::OnKillFocus(wxFocusEvent& evt) {
  text_window_->OnTextKillFocus(evt);
  evt.Skip();  // Skip for default handling.
}

}  // namespace editor
