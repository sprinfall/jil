#ifndef JIL_EDITOR_STATUS_BAR_H_
#define JIL_EDITOR_STATUS_BAR_H_
#pragma once

#include <vector>
#include "wx/panel.h"
#include "editor/theme.h"

namespace jil {
namespace editor {

class StatusBar : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BORDER = 0,
    FG,
    BG,
    COLOR_COUNT
  };

  enum FontId {
    FONT = 0,
    FONT_COUNT
  };

  enum FieldId {
    kField_Cwd = 0,
    kField_Encoding,
    kField_FileFormat,
    kField_FileType,
    kField_Caret,
    kField_KeyStroke,
    kField_Space,

    kField_Count,
  };

  enum SizeType {
    kFit,
    kFixedPixel,
    kFixedPercentage,
    kStretch
  };

  struct FieldInfo {
    FieldId id;
    wxAlignment align;
    SizeType size_type;
    // For different size types, different meanings of size value:
    // kFit -> padding
    // kFixedPixel -> pixels
    // kFixedPercentage -> % x 100
    // kStretch -> stretch factor
    int size_value;
    // Actually size.
    int size;
  };

public:
  StatusBar();
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~StatusBar();

  void set_theme(const SharedTheme& theme) {
    theme_ = theme;
  }

  void AddField(FieldId id,
                wxAlignment align,
                SizeType size_type,
                int size_value);

  void SetFields(const std::vector<FieldInfo>& field_infos);

  void UpdateFieldSizes();

  void SetFieldValue(FieldId id, const wxString& value, bool refresh);

  void ClearFieldValues();

protected:
  virtual wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);
  void DrawBackground(wxDC& dc, const wxRect& bg_rect);

  void OnSize(wxSizeEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);

private:
  wxString GetFieldValue(FieldId id);

  // Get field rect according to its size and the client rect.
  // If the field is not found, the rect will be empty.
  wxRect GetFieldRect(FieldId id) const;

  const FieldInfo* GetFieldByPos(int pos_x) const;

private:
  SharedTheme theme_;

  std::vector<FieldInfo> field_infos_;

  wxString field_values_[kField_Count];

  int char_height_;
};

////////////////////////////////////////////////////////////////////////////////

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(kEvtStatusFieldClick, 0)
END_DECLARE_EVENT_TYPES()

}  // namespace editor
}  // namespace jil

#define EVT_STATUS_FIELD_CLICK(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(::jil::editor::kEvtStatusFieldClick, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_EDITOR_STATUS_BAR_H_
