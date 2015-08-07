#ifndef JIL_UI_FONT_PREVIEW_CTRL_H_
#define JIL_UI_FONT_PREVIEW_CTRL_H_
#pragma once

#include "wx/control.h"

namespace jil {
namespace ui {

// FontPreviewCtrl displays the font face with the font style. 
class FontPreviewCtrl : public wxControl {
  DECLARE_EVENT_TABLE()

public:
  FontPreviewCtrl(wxWindow* parent, wxWindowID id, const wxFont& font);
  virtual ~FontPreviewCtrl();

protected:
  virtual wxSize DoGetBestSize() const override;

  void OnPaint(wxPaintEvent& evt);
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_FONT_PREVIEW_CTRL_H_
