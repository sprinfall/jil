#ifndef JIL_UI_BITMAP_BUTTON_H_
#define JIL_UI_BITMAP_BUTTON_H_
#pragma once

// A replacement of wxBitmapButton.

#include "ui/button_base.h"
#include "wx/bitmap.h"

namespace jil {
namespace ui {

class BitmapButton : public ButtonBase {
  DECLARE_CLASS(BitmapButton)
  DECLARE_NO_COPY_CLASS(BitmapButton)

public:
  explicit BitmapButton(SharedButtonStyle style);
  virtual ~BitmapButton();

  bool Create(wxWindow* parent, wxWindowID id);

  void set_user_best_size(const wxSize& user_best_size) {
    user_best_size_ = user_best_size;
  }

  void SetBitmap(const wxBitmap& bitmap) {
    bitmap_ = bitmap;
  }

protected:
  virtual wxSize DoGetBestSize() const override;

  virtual ButtonStyle::State GetState() const override;

  virtual void DrawForeground(wxDC& dc, ButtonStyle::State state) override;

  virtual void PostEvent() override;

private:
  wxSize user_best_size_;
  wxBitmap bitmap_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_BITMAP_BUTTON_H_
