#ifndef JIL_EDITOR_TIP_H_
#define JIL_EDITOR_TIP_H_
#pragma once

// Customized tool tip.

#include <vector>
#include <memory>
#include "wx/frame.h"
#include "wx/timer.h"

namespace jil {
namespace editor {

struct TipModel {
  void SetTip(const wxString& tip_str);

  bool Valid() const {
    return !tips.empty();
  }

  std::vector<wxString> tips;
};

struct TipTheme {
  wxColour bg;
  wxColour fg;
  wxColour border;
};

class TipHandler;

class Tip : public wxFrame {
  DECLARE_NO_COPY_CLASS(Tip)

 public:
  Tip(TipModel* model, wxWindow* parent);
  virtual ~Tip();

  const TipTheme& theme() const { return theme_; }
  void set_theme(const TipTheme& theme) { theme_ = theme; }

 private:
  TipModel* model_;
  TipTheme theme_;
};

// Handle tip for a window.
// Usage:
// - keep TipHandler as a member variable of your window class
// - call PushEventHandler() when your window is created (mostly, in ctor)
// - call PopEventHandler() when your window is destroyed (mostly, in dtor)
class TipHandler : public wxEvtHandler {
  DECLARE_EVENT_TABLE()

 public:
  // Owner window is the one who has the tip.
  explicit TipHandler(wxWindow* owner);
  virtual ~TipHandler();

  void SetTip(const wxString& tip_str);

  void set_interval_ms(int interval_ms) {
    interval_ms_ = interval_ms;
  }

  void set_start_on_move(bool start_on_move) {
    start_on_move_ = start_on_move;
  }

  void set_tip_theme(const TipTheme& tip_theme) {
    tip_theme_ = tip_theme;
  }

 protected:
  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseMotion(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseEnter(wxMouseEvent& evt);
  void OnMouseLeave(wxMouseEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);

  void OnTipTimer(wxTimerEvent& evt);

  virtual void HandleMouseLeftDown(wxMouseEvent& evt);
  virtual void HandleMouseMove(wxMouseEvent& evt);
  virtual void HandleMouseLeftUp(wxMouseEvent& evt);
  virtual void HandleMouseEnter(wxMouseEvent& evt);
  virtual void HandleMouseLeave(wxMouseEvent& evt);
  virtual void HandleKillFocus(wxFocusEvent& evt);

  void StartTip();
  void CloseTip();

 protected:
  wxWindow* owner_window_;
  Tip* tip_;
  TipTheme tip_theme_;
  TipModel tip_model_;
  int interval_ms_;  // Time interval in ms.
  wxTimer tip_timer_;
  bool start_on_move_;  // Start timer on mouse move.
};

}  // namespace editor
}  // namespace jil

#endif  // JIL_EDITOR_TIP_H_
