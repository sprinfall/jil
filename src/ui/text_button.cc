#include "ui/text_button.h"
#include "wx/dcclient.h"

namespace jil {
namespace ui {

IMPLEMENT_CLASS(TextButton, ButtonBase);

TextButton::TextButton(SharedButtonStyle style)
    : ButtonBase(style) {
}

TextButton::~TextButton() {
}

bool TextButton::Create(wxWindow* parent,
                        wxWindowID id,
                        const wxString& label) {
  if (!ButtonBase::Create(parent, id)) {
    return false;
  }

  SetLabel(label);

  return true;
}

wxSize TextButton::DoGetBestSize() const {
  if (min_size_ != wxDefaultSize) {
    return min_size_;
  }

  wxSize best_size;
  GetTextExtent(GetLabel(), &best_size.x, &best_size.y);
  best_size += padding_;
  best_size += padding_;
  return best_size;
}

ButtonStyle::State TextButton::GetState() const {
  if (!IsThisEnabled()) {  // NOTE: Don't use IsEnabled()!
    return ButtonStyle::DISABLED;
  }

  if (pressed_) {
    return ButtonStyle::PRESSED;
  } else if (hover_) {
    return ButtonStyle::HOVER;
  } else {
    return ButtonStyle::NORMAL;
  }
}

void TextButton::DrawFg(wxDC& dc, ButtonStyle::State state) {
  dc.SetFont(GetFont());

  wxColour fg = style_->GetColor(ButtonStyle::FG, state);
  dc.SetTextForeground(fg);

  dc.DrawLabel(GetLabel(), GetClientRect(), wxALIGN_CENTER);
}

void TextButton::PostEvent() {
  wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  evt.SetEventObject(this);
  evt.SetString(GetLabel());
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}

}  // namespace ui
}  // namespace jil
