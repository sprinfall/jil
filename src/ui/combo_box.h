#ifndef JIL_UI_COMBO_BOX_H_
#define JIL_UI_COMBO_BOX_H_
#pragma once

#include <vector>
#include "boost/shared_ptr.hpp"
#include "wx/colour.h"
#include "wx/control.h"
#include "wx/dialog.h"

class wxGraphicsContext;

namespace jil {
namespace ui {

////////////////////////////////////////////////////////////////////////////////

class ComboStyle {
public:
  enum Part {
    BG = 0,
    FG,
    BORDER,
    ARROW_BORDER,
    ARROW_FILL,
    PARTS,
  };

  enum State {
    NORMAL = 0,
    NORMAL_HOVER,
    PRESSED,
    PRESSED_HOVER,
    STATES,
  };

public:
  const wxColour& GetColor(int part, int state) const {
    assert(part >= 0 && part < PARTS);
    assert(state >= 0 && state < STATES);
    return colors_[part][state];
  }

  void SetColor(int part, int state, const wxColour& color) {
    assert(part >= 0 && part < PARTS);
    assert(state >= 0 && state < STATES);
    colors_[part][state] = color;
  }

  // Fix missing colors.
  void Fix();

private:
  wxColour colors_[PARTS][STATES];
};

typedef boost::shared_ptr<ComboStyle> SharedComboStyle;

////////////////////////////////////////////////////////////////////////////////

class ComboPopup : public wxDialog {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    BG = 0,
    FG,
    BG_HOVER,
    FG_HOVER,
    BORDER,
    SEPARATOR,
    COLORS,
  };

public:
  ComboPopup();
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~ComboPopup();

  void set_padding(const wxSize& padding) {
    padding_ = padding;
  }

  void SetColor(ColorId color_id, const wxColour& color) {
    assert(color_id < COLORS);
    colors_[color_id] = color;
  }

  void Append(const wxString& label);

  void SetLabels(const std::vector<wxString>& labels);

  // Set client size according to content.
  void AdjustSize(int min_width);

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnActivate(wxActivateEvent& evt);
  void OnMouseMotion(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseLeave(wxMouseEvent& evt);
  void OnKeyDownHook(wxKeyEvent& evt);

private:
  // Return -1 if no item under the given position or the item is a separator.
  size_t IndexItem(const wxPoint& pos);

  // Refresh an item at the given index.
  void RefreshByIndex(size_t index);

private:
  std::vector<wxString> labels_;
  size_t hover_index_;
  std::vector<wxRect> item_rects_;

  wxSize margin_;
  wxSize padding_;

  wxColour colors_[COLORS];
};

////////////////////////////////////////////////////////////////////////////////

class ComboBox : public wxControl {
  DECLARE_EVENT_TABLE()

public:
  ComboBox(SharedComboStyle style);
  bool Create(wxWindow* parent, wxWindowID id, const wxString& label);
  virtual ~ComboBox();

  virtual bool AcceptsFocus() const override {
    return accept_focus_;
  }

  void set_accept_focus(bool accept_focus) {
    accept_focus_ = accept_focus;
  }

  void Append(const wxString& label);

protected:
  virtual wxSize DoGetBestSize() const;

  ComboStyle::State GetState() const;

  void OnPaint(wxPaintEvent& evt);
  void DrawArrow(wxGraphicsContext* gc, ComboStyle::State state);

  void OnMouseLeftDown(wxMouseEvent& evt);
  void OnMouseLeftUp(wxMouseEvent& evt);
  void OnMouseEnter(wxMouseEvent& evt);
  void OnMouseLeave(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);
  void OnSetFocus(wxFocusEvent& evt);
  void OnKillFocus(wxFocusEvent& evt);
  void OnKeyUp(wxKeyEvent& evt);

  void OnPopupClose(wxCommandEvent& evt);
  void OnPopupSelected(wxCommandEvent& evt);

  void ShowPopup();
  void ClosePopup();

  void ConnectPopupEventHandlers();
  void DisconnectPopupEventHandlers();

private:
  SharedComboStyle style_;

  wxSize padding_;
  wxSize arrow_size_;

  bool pressed_;
  bool hover_;

  bool accept_focus_;
  bool down_;

  std::vector<wxString> labels_;
  ComboPopup* popup_;
};

}  // namespace ui
}  // namespace jil

#endif  // JIL_UI_COMBO_BOX_H_
