#ifndef JIL_TOOL_BOOK_H_
#define JIL_TOOL_BOOK_H_
#pragma once

#include "app/book_ctrl.h"

namespace jil {

class ToolBook : public BookCtrl {
  DECLARE_EVENT_TABLE()

public:
  ToolBook();
  virtual ~ToolBook();

  bool Create(wxWindow* parent, wxWindowID id);

  wxPanel* page_area() const {
    return page_panel_;
  }

protected:
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt) override;

  virtual void DoActivateTab(Tab* tab, bool active) override;
  virtual void DoRemoveTab(Tab* tab, bool from_remove_all) override;

  // Handlers for right click menu items on the tab area.
  void OnMenuClose(wxCommandEvent& evt);
  void OnMenuCloseAll(wxCommandEvent& evt);
  void OnMenuCloseAllButThis(wxCommandEvent& evt);
};

}  // namespace jil

#endif  // JIL_TOOL_BOOK_H_
