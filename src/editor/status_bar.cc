#include "editor/status_bar.h"
#include "wx/dcbuffer.h"
#include "wx/menu.h"
#include "wx/log.h"
#include "ui/color.h"
#include "editor/text_extent.h"
#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"

namespace jil {
namespace editor {

BEGIN_EVENT_TABLE(StatusBar, wxPanel)
EVT_PAINT     (StatusBar::OnPaint)
EVT_SIZE      (StatusBar::OnSize)
EVT_LEFT_DOWN (StatusBar::OnMouseLeftDown)
END_EVENT_TABLE()

StatusBar::StatusBar()
    : char_height_(0) {
}

bool StatusBar::Create(wxWindow* parent, wxWindowID id) {
  assert(theme_);

  if (!wxPanel::Create(parent, id)) {
    return false;
  }

  SetBackgroundStyle(wxBG_STYLE_PAINT);

  if (font_.IsOk()) {
    SetFont(font_);
  }

  // Cache the char height after set font.
  char_height_ = GetCharHeight();

  // Determine padding by char width.
  int char_width = GetCharWidth();
  padding_.Set(char_width, char_width / 2 + 1);

  return true;
}

StatusBar::~StatusBar() {
}

void StatusBar::AddField(FieldId id,
                         wxAlignment align,
                         SizeType size_type,
                         int size_value) {
  FieldInfo field_info = { id, align, size_type, size_value, 0 };
  field_infos_.push_back(field_info);
}

void StatusBar::SetFields(const std::vector<FieldInfo>& field_infos) {
  field_infos_ = field_infos;
}

void StatusBar::UpdateFieldSizes() {
  const int client_size = GetClientSize().x;
  int size_left = client_size;

  int stretch_field_count = 0;  // The number of stretch fields.

  for (size_t i = 0; i < field_infos_.size(); ++i) {
    FieldInfo& field_info = field_infos_[i];

    switch (field_info.size_type) {
      case kFit:
        GetTextExtent(GetFieldValue(field_info.id),
                      &field_info.size,
                      NULL,
                      NULL,
                      NULL);
        field_info.size += field_info.size_value;
        field_info.size += padding_.x + padding_.x;
        break;

      case kFixedPixel:
        field_info.size = field_info.size_value;
        break;

      case kFixedPercentage:
        field_info.size = field_info.size_value * client_size / 100;
        break;

      case kStretch:
        // Reset size. Set it later.
        field_info.size = 0;
        ++stretch_field_count;
        break;
    }

    if (field_info.size > 0) {
      size_left -= field_info.size;
    }
  }

  // Set size for stretch fields.

  if (stretch_field_count > 0 && size_left > 0) {
    int stretch_size = size_left / stretch_field_count;

    if (stretch_size > 0) {
      for (size_t i = 0; i < field_infos_.size(); ++i) {
        if (field_infos_[i].size_type == kStretch) {
          field_infos_[i].size = stretch_size;
        }
      }
    }

    size_left = size_left % stretch_field_count;

    if (size_left > 0) {
      // Give it to the first stretch field.
      for (size_t i = 0; i < field_infos_.size(); ++i) {
        if (field_infos_[i].size_type == kStretch) {
          field_infos_[i].size += size_left;
          break;
        }
      }
    }
  }
}

void StatusBar::SetFieldValue(FieldId id,
                              const wxString& value,
                              bool refresh) {
  field_values_[id] = value;

  wxRect field_rect = GetFieldRect(id);
  if (!field_rect.IsEmpty()) {
    RefreshRect(field_rect);
  }
}

void StatusBar::ClearFieldValues() {
  for (size_t i = 0; i < kField_Count; ++i) {
    field_values_[i] = wxEmptyString;
  }
}

wxSize StatusBar::DoGetBestSize() const {
  return wxSize(-1, char_height_ + padding_.y + padding_.y);
}

void StatusBar::OnPaint(wxPaintEvent& evt) {
  wxAutoBufferedPaintDC dc(this);
#if !wxALWAYS_NATIVE_DOUBLE_BUFFER
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
#endif

  const wxRect rect = GetClientRect();

  dc.SetTextForeground(theme_->GetColor(FG));
  dc.SetFont(GetFont());

  wxRect update_rect = GetUpdateClientRect();
  update_rect.SetTop(rect.GetTop());
  update_rect.SetBottom(rect.GetBottom());

  // Background
  DrawBackground(dc, update_rect);

  // Foreground
  dc.SetBrush(*wxTRANSPARENT_BRUSH);

  int label_y = rect.GetTop() + (rect.GetHeight() - char_height_) / 2;

  int x = rect.GetLeft();

  for (size_t i = 0; i < field_infos_.size(); ++i) {
    FieldInfo& field_info = field_infos_[i];
    wxRect field_rect(x, rect.GetTop(), field_info.size, rect.GetHeight());

    if (field_rect.GetLeft() >= update_rect.GetRight()) {
      // At the right of update rect.
      break;
    }

    if (field_rect.GetRight() <= update_rect.GetLeft()) {
      // At the left of update rect.
      x += field_info.size;
      continue;
    }

    wxString label = GetFieldValue(field_info.id);
    field_rect.Deflate(padding_.x, 0);

    int expected_size = 0;
    dc.GetTextExtent(label, &expected_size, NULL, NULL, NULL);

    if (expected_size > field_rect.GetWidth()) {
      label = label.Mid(0, TailorLabel(dc, label, field_rect.GetWidth()));
    }

    dc.DrawLabel(label,
                 field_rect,
                 field_info.align | wxALIGN_CENTER_VERTICAL);

    x += field_info.size;
  }
}

void StatusBar::DrawBackground(wxDC& dc, const wxRect& bg_rect) {
  wxColour bg_bottom = theme_->GetColor(BG);

  int gradient_color_delta = (bg_rect.GetHeight() + 2) / 3;
  wxColour bg_top = ui::IncColor(bg_bottom, gradient_color_delta);

  dc.GradientFillLinear(bg_rect, bg_bottom, bg_top, wxNORTH);

  dc.SetPen(wxPen(theme_->GetColor(BORDER)));
  dc.DrawLine(bg_rect.GetLeft(),
              bg_rect.GetTop(),
              bg_rect.GetRight() + 1,
              bg_rect.GetTop());
}

void StatusBar::OnSize(wxSizeEvent& evt) {
  evt.Skip();

  if (GetClientSize().x < kUnreadyWindowSize) {
    return;
  }

  UpdateFieldSizes();
  Refresh();
}

void StatusBar::OnMouseLeftDown(wxMouseEvent& evt) {
  const FieldInfo* field_info = GetFieldByPos(evt.GetPosition().x);
  if (field_info == NULL) {
    return;
  }

  wxCommandEvent field_click_evt(kEvtStatusFieldClick, GetId());
  field_click_evt.SetEventObject(this);
  field_click_evt.SetInt(field_info->id);
  GetParent()->GetEventHandler()->AddPendingEvent(field_click_evt);
}

wxString StatusBar::GetFieldValue(FieldId id) {
  return field_values_[id];
}

wxRect StatusBar::GetFieldRect(FieldId id) const {
  const wxRect client_rect = GetClientRect();
  int x = client_rect.GetLeft();

  for (size_t i = 0; i < field_infos_.size(); ++i) {
    const FieldInfo& field_info = field_infos_[i];
    if (field_info.id == id) {
      return wxRect(x,
                    client_rect.GetTop(),
                    field_info.size,
                    client_rect.GetHeight());
    }
    x += field_info.size;
  }

  return wxRect();
}

const StatusBar::FieldInfo* StatusBar::GetFieldByPos(int pos_x) const {
  int x = GetClientRect().GetLeft();
  for (size_t i = 0; i < field_infos_.size(); ++i) {
    if (pos_x > x && pos_x < x + field_infos_[i].size) {
      return &field_infos_[i];
    }
    x += field_infos_[i].size;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

DEFINE_EVENT_TYPE(kEvtStatusFieldClick);

}  // namespace editor
}  // namespace jil
