#ifndef JIL_UI_BITMAP_BUTTON_BASE_H_
#define JIL_UI_BITMAP_BUTTON_BASE_H_
#pragma once

#include "wx/bitmap.h"
#include "wx/control.h"
#include "ui/button_base.h"

namespace jil {
namespace ui {

class BitmapButtonBase : public ButtonBase {
  DECLARE_CLASS(BitmapButtonBase)
  DECLARE_NO_COPY_CLASS(BitmapButtonBase)

public:
  explicit BitmapButtonBase(SharedButtonStyle style);
  virtual ~BitmapButtonBase();

  bool Create(wxWindow* parent, wxWindowID id);

  void SetBitmap(ButtonStyle::State state, const wxBitmap& bitmap) {
    bitmaps_[state] = bitmap;
  }

  void SetBitmaps(const wxBitmap& normal,
                  const wxBitmap& hover,
                  const wxBitmap& pressed,
                  const wxBitmap& disabled);

  void SetBitmaps(const wxBitmap& bitmap);

  void SetBitmapsND(const wxBitmap& normal, const wxBitmap& disabled);

protected:
  wxSize DoGetBestSize() const override;

  virtual void DrawFg(wxDC& dc, ButtonStyle::State state) override;

  virtual void PostEvent() = 0;

protected:
  wxBitmap bitmaps_[ButtonStyle::STATES];
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BITMAP_BUTTON_BASE_H_
