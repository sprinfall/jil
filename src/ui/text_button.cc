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
  if (!ButtonBase::Create(parent, id, label)) {
    return false;
  }

  return true;
}

wxSize TextButton::DoGetBestSize() const {
  wxSize best_size;
  GetTextExtent(GetLabel(), &best_size.x, &best_size.y);
  best_size.IncBy(padding_.x, padding_.y);

  wxSize min_size = GetMinSize();
  if (best_size.x < min_size.x) {
    best_size.x = min_size.x;
  }
  if (best_size.y < min_size.y) {
    best_size.y = min_size.y;
  }

  return best_size;
}

ButtonStyle::State TextButton::GetState() {
  if (!IsEnabled()) {
    return ButtonStyle::DISABLED;
  }

  if (pressed_) {
    return hover_ ? ButtonStyle::PRESSED_HOVER : ButtonStyle::PRESSED;
  } else {
    return hover_ ? ButtonStyle::NORMAL_HOVER : ButtonStyle::NORMAL;
  }
}

void TextButton::DrawForeground(wxDC& dc, ButtonStyle::State state) {
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
