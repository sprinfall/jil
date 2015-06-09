#ifndef JIL_TOOL_BOOK_H_
#define JIL_TOOL_BOOK_H_
#pragma once

#include "app/book_ctrl.h"

namespace jil {

class ToolBook : public BookCtrl {
  DECLARE_EVENT_TABLE()

public:
  explicit ToolBook(const editor::SharedTheme& theme);
  bool Create(wxWindow* parent, wxWindowID id);
  virtual ~ToolBook();

protected:
  virtual void HandleTabMouseRightUp(wxMouseEvent& evt) override;

  void OnMenuClose(wxCommandEvent& evt);
};

}  // namespace jil

#endif  // JIL_TOOL_BOOK_H_
