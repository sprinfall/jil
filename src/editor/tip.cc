#include "editor/tip.h"
#include "wx/tokenzr.h"
#include "wx/sizer.h"
#include "wx/panel.h"
#include "wx/dcbuffer.h"
#include "wx/display.h"
#include "wx/log.h"

namespace jil {
namespace editor {

////////////////////////////////////////////////////////////////////////////////

void TipModel::SetTip(const wxString& tip_str) {
  tips.clear();

  if (!tip_str.empty()) {
    wxStringTokenizer tokenizer(tip_str, _T("\n"));
    while (tokenizer.HasMoreTokens()) {
      wxString tip = tokenizer.GetNextToken();
      if (tip.find_first_not_of(_T(' ')) != wxString::npos) {
        tips.push_back(tip);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static const int kPaddingX = 7;
static const int kPaddingY = 3;

class TipBody : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  TipBody(Tip* tip, TipModel* tip_data_)
      : wxPanel(tip, wxID_ANY), tip_(tip), tip_model_(tip_data_)
      , line_height_(0) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    GetTextExtent(wxT("T"), NULL, &line_height_, NULL, NULL);
  }

protected:
  virtual wxSize DoGetBestSize() const {
    int max_width = 0;

    for (size_t i = 0; i < tip_model_->tips.size(); ++i) {
      int tip_w = 0;
      GetTextExtent(tip_model_->tips[i], &tip_w, NULL, NULL, NULL);

      if (max_width < tip_w) {
        max_width = tip_w;
      }
    }

    int w = max_width + kPaddingX + kPaddingX;
    int h = (line_height_ + kPaddingY) * tip_model_->tips.size() + kPaddingY;

    return wxSize(w, h);
  }

  void OnPaint(wxPaintEvent& evt) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(tip_->theme().bg));
    dc.SetTextForeground(tip_->theme().fg);
    dc.SetFont(GetFont());
    dc.Clear();

    size_t rows = 0;

    int y = kPaddingY;
    for (size_t i = 0; i < tip_model_->tips.size(); ++i) {
      dc.DrawText(tip_model_->tips[i], kPaddingX, y);
      ++rows;
      y += line_height_ + kPaddingY;
    }
  }

private:
  Tip* tip_;
  TipModel* tip_model_;
  int line_height_;
};

BEGIN_EVENT_TABLE(TipBody, wxPanel)
  EVT_PAINT(TipBody::OnPaint)
END_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////

#if defined (__WXMAC__)
#define TOOLTIP_STYLE wxPOPUP_WINDOW | wxSTAY_ON_TOP | wxBORDER_NONE
#else
#define TOOLTIP_STYLE wxFRAME_TOOL_WINDOW | wxSTAY_ON_TOP | wxBORDER_NONE
#endif

////////////////////////////////////////////////////////////////////////////

Tip::Tip(TipModel* model, wxWindow* parent)
    : wxFrame(parent,
              wxID_ANY,
              wxEmptyString,
              wxDefaultPosition,
              wxDefaultSize,
              TOOLTIP_STYLE)
    , model_(model) {
  SetBackgroundColour(theme_.bg);

  TipBody* body = new TipBody(this, model_);

  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(body, 0, wxALL, 1);
  SetSizer(sizer);
  sizer->Fit(this);
}

Tip::~Tip() {
}

////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(TipHandler, wxEvtHandler)
EVT_LEFT_DOWN     (TipHandler::OnMouseLeftDown)
EVT_MOTION        (TipHandler::OnMouseMotion)
EVT_LEFT_UP       (TipHandler::OnMouseLeftUp)
EVT_ENTER_WINDOW  (TipHandler::OnMouseEnter)
EVT_LEAVE_WINDOW  (TipHandler::OnMouseLeave)
EVT_KILL_FOCUS    (TipHandler::OnKillFocus)
END_EVENT_TABLE()

TipHandler::TipHandler(wxWindow* owner)
    : owner_window_(owner)
    , tip_(NULL)
    , interval_ms_(500) // 500ms by default.
    , tip_timer_(this, wxID_ANY)
    , start_on_move_(false) {

  tip_theme_.bg = *wxWHITE;
  tip_theme_.fg = *wxBLACK;
  tip_theme_.border = *wxLIGHT_GREY;

  Connect(tip_timer_.GetId(),
          wxEVT_TIMER,
          wxTimerEventHandler(TipHandler::OnTipTimer));
}

TipHandler::~TipHandler() {
  Disconnect(tip_timer_.GetId(),
             wxEVT_TIMER,
             wxTimerEventHandler(TipHandler::OnTipTimer));
}

void TipHandler::SetTip(const wxString& tip_str) {
  tip_model_.SetTip(tip_str);
}

void TipHandler::OnMouseLeftDown(wxMouseEvent& evt) {
  HandleMouseLeftDown(evt);
  evt.Skip();
}

void TipHandler::OnMouseMotion(wxMouseEvent& evt) {
  HandleMouseMove(evt);
  evt.Skip();
}

void TipHandler::OnMouseLeftUp(wxMouseEvent& evt) {
  HandleMouseLeftUp(evt);
  evt.Skip();
}

void TipHandler::OnMouseEnter(wxMouseEvent& evt) {
  HandleMouseEnter(evt);
  evt.Skip();
}

void TipHandler::OnMouseLeave(wxMouseEvent& evt) {
  HandleMouseLeave(evt);
  evt.Skip();
}

void TipHandler::OnKillFocus(wxFocusEvent& evt) {
  HandleKillFocus(evt);
}

void TipHandler::HandleMouseLeftDown(wxMouseEvent& WXUNUSED(evt)) {
  CloseTip();
}

void TipHandler::HandleMouseMove(wxMouseEvent& WXUNUSED(evt)) {
  if (start_on_move_) {
    StartTip();
  }
}

void TipHandler::HandleMouseLeftUp(wxMouseEvent& evt) {
  if (owner_window_->GetClientRect().Contains(evt.GetPosition())) {
    StartTip();
  }
}

void TipHandler::HandleMouseEnter(wxMouseEvent& WXUNUSED(evt)) {
  StartTip();
}

void TipHandler::HandleMouseLeave(wxMouseEvent& WXUNUSED(evt)) {
  CloseTip();
}

void TipHandler::HandleKillFocus(wxFocusEvent& WXUNUSED(evt)) {
  CloseTip();
}

void TipHandler::StartTip() {
  CloseTip();

  if (!tip_model_.Valid()) {
    return;
  }

  tip_timer_.Start(interval_ms_, true);
}

void TipHandler::CloseTip() {
  if (tip_timer_.IsRunning()) {
    tip_timer_.Stop();
  }

  if (tip_ != NULL) {
    tip_->Destroy();
    tip_ = NULL;
  }
}

static wxPoint CalcBestPosition(wxWindow* owner, const wxSize& tip_size);

void TipHandler::OnTipTimer(wxTimerEvent& WXUNUSED(evt)) {
  tip_ = new Tip(&tip_model_, owner_window_);
  tip_->set_theme(tip_theme_);
  wxPoint p = CalcBestPosition(owner_window_, tip_->GetBestSize());
  tip_->SetPosition(p);
  tip_->Show();
}

////////////////////////////////////////////////////////////////////////////

static const int YUp = 4;
static const int YDown = 24;

static wxPoint CalcBestPosition(wxWindow* owner, const wxSize& tip_size) {
  wxPoint pos = ::wxGetMousePosition();

  int display_index = wxDisplay::GetFromWindow(owner);
  wxDisplay display(display_index == wxNOT_FOUND ? 0 : display_index);
  const wxRect screen_rect(display.GetClientArea());

  // Try down.
  pos.y += YDown;

  int dy = pos.y + tip_size.GetHeight() - screen_rect.GetBottom();
  int dx = pos.x + tip_size.GetWidth() - screen_rect.GetRight();

  if (dy <= 0 && dx <= 0) {
    return pos;
  }

  if (dx > 0) {
    // Move tip left by dx.
    pos.x -= dx;
  }

  // Try up.
  if (dy > 0) {
    pos.y -= YDown + YUp + tip_size.GetHeight();
  }

  return pos;
}

}  // namespace editor
}  // namespace jil
