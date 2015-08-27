#ifndef JIL_OPTION_LIST_CTRL_H
#define JIL_OPTION_LIST_CTRL_H
#pragma once

// A replacement of wxPropertyGrid.

#include <vector>
#include "boost/any.hpp"
#include "wx/scrolwin.h"
#include "wx/panel.h"
#include "editor/option.h"

class wxTextCtrl;

namespace jil {

class OptionListCtrl;

////////////////////////////////////////////////////////////////////////////////

class OlcHeadPanel : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  OlcHeadPanel(OptionListCtrl* option_list_ctrl, wxWindowID id);
  virtual ~OlcHeadPanel();

  // Let body panel have the focus.
  virtual bool AcceptsFocus() const override {
    return false;
  }

protected:
  void OnPaint(wxPaintEvent& evt);
  void OnMouseEvents(wxMouseEvent& evt);
  void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt);

private:
  OptionListCtrl* option_list_ctrl_;
};

////////////////////////////////////////////////////////////////////////////////

class OlcBodyPanel : public wxPanel {
  DECLARE_EVENT_TABLE()

public:
  OlcBodyPanel(OptionListCtrl* option_list_ctrl, wxWindowID id);
  virtual ~OlcBodyPanel();

protected:
  void OnSize(wxSizeEvent& evt);
  void OnPaint(wxPaintEvent& evt);
  void OnMouseLeftDown(wxMouseEvent& evt);

private:
  OptionListCtrl* option_list_ctrl_;
};

////////////////////////////////////////////////////////////////////////////////

class OptionListCtrl : public wxScrolledWindow {
  DECLARE_EVENT_TABLE()

public:
  enum ColorId {
    COLOR_HEAD_FG = 0,
    COLOR_HEAD_BG,
    COLOR_HEAD_BORDER,
    COLOR_BODY_BG,
    COLOR_BODY_BG_SELECT,
    COLOR_BODY_FG,
    COLOR_BODY_FG_SELECT,
    COLOR_BODY_BORDER,
    COLOR_COUNT
  };

public:
  OptionListCtrl();
  virtual ~OptionListCtrl();

  bool Create(wxWindow* parent,
              wxWindowID id,
              const wxSize& size = wxDefaultSize,
              long style = wxScrolledWindowStyle);

  const wxColour& GetColor(ColorId id) const {
    return colors_[id];
  }
  void SetColor(ColorId id, const wxColour& color) {
    colors_[id] = color;
  }

  void StartBatch();
  void EndBatch();

  void AddOption(const editor::OptionPair& option_pair);

  const editor::OptionPair* GetOption(int i) const;

  int GetCount() const;

protected:
  void Init();

  void CheckColors();

  editor::OptionPair* GetOptionByRow(int row);

  void OnSize(wxSizeEvent& evt);

  void OnEditingDone(wxCommandEvent& evt);

  friend class OlcHeadPanel;
  void OnHeadPaint(wxDC& dc);
  void OnHeadMouse(wxMouseEvent& evt);

  friend class OlcBodyPanel;
  void OnBodySize(wxSizeEvent& evt);
  void OnBodyPaint(wxDC& dc);
  void OnBodyMouseLeftDown(wxMouseEvent& evt);

  bool IsEditing() const;
  void StartEditing(int row);
  void FinishEditing();

  wxRect GetRowClientRect(int row) const;

  void RefreshRow(int row);
  void RefreshRowByKey(const std::string& key);

  int GetColumnX(int col) const;
  int GetColumnWidth(int col) const;
  wxRect GetColumnRect(int row, int col) const;

  int GetScrolledX(int unscrolled_x) const;
  int GetScrolledY(int unscrolled_y) const;

  void UpdateLayout();
  void UpdateVirtualSize();

private:
  class Column {
  public:
    enum {
      KEY = 0,
      VALUE,
      COUNT,
    };

    wxString label;
    int width;
  };

private:
  bool created_;
  bool batch_;

  wxColour colors_[COLOR_COUNT];

  OlcHeadPanel* head_panel_;
  OlcBodyPanel* body_panel_;

  wxSize row_padding_;
  int row_height_;

  // Key, value columns.
  Column columns_[2];

  std::vector<editor::OptionPair*> options_;
  int selected_row_;

  wxTextCtrl* text_ctrl_;
};

}  // namespace jil

#endif  // JIL_OPTION_LIST_CTRL_H
