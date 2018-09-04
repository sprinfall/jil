#ifndef JIL_BOOK_PAGE_H_
#define JIL_BOOK_PAGE_H_

#include "wx/string.h"

class wxMenu;
class wxWindow;

namespace jil {

////////////////////////////////////////////////////////////////////////////////

// Page interface of book ctrl.
class BookPage {
public:
  enum Flag {
    kModified = 1,
    kNewCreated = 2,
  };

  virtual ~BookPage() {
  }

  // NOTE: Prefix "Page_" is added to avoid name conflict.

  virtual bool Page_HasFocus() const = 0;
  virtual void Page_SetFocus() = 0;

  // Activate/deactivate this page.
  virtual void Page_Activate(bool active) = 0;

  // Close this page (also destroy the window).
  virtual void Page_Close() = 0;

  // Page type ID.
  virtual int Page_Type() const = 0;

  // Page label displayed in tab.
  virtual wxString Page_Label() const = 0;

  // Page description displayed, e.g., in tab tooltip.
  virtual wxString Page_Description() const {
    return wxEmptyString;
  }

  // See enum Flag.
  virtual int Page_Flags() const {
    return 0;
  }

  // Add menu items to the edit menu.
  // Different pages might have different edit menu items.
  // E.g., text page has Undo and Redo while find result page doesn't.
  virtual void Page_EditMenu(wxMenu* menu) = 0;

  // Get the enable state of the edit menu item.
  virtual bool Page_EditMenuState(int menu_id) = 0;

  // Get the enable state of the file menu item.
  virtual bool Page_FileMenuState(int menu_id) = 0;

  // Handle the menu event.
  virtual bool Page_OnMenu(int menu_id) = 0;

  virtual bool Page_Save() = 0;

  bool Page_IsModified() const {
    return (Page_Flags() & kModified) != 0;
  }

  bool Page_IsNewCreated() const {
    return (Page_Flags() & kNewCreated) != 0;
  }
};

}  // namespace jil

#endif  // JIL_BOOK_PAGE_H_
