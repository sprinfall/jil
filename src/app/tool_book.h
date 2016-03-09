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
    return page_area_;
  }

protected:
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt) override;

  virtual void DoActivateTab(Tab* tab, bool active) override;
  virtual void DoRemoveTab(Tab* tab) override;
  virtual void DoRemoveAll(Tab* tab) override;

  void OnMenuClose(wxCommandEvent& evt);
};

}  // namespace jil

#endif  // JIL_TOOL_BOOK_H_
