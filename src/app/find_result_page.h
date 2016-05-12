#ifndef JIL_FIND_RESULT_PAGE_H_
#define JIL_FIND_RESULT_PAGE_H_
#pragma once

// Text window as find result page in the tool book.

#include "editor/text_window.h"
#include "app/book_page.h"

namespace jil {

BEGIN_DECLARE_EVENT_TYPES()
// Check GetInt(), which returns enum FindResultPage::EventType, for the details.
DECLARE_EVENT_TYPE(kFindResultPageEvent, 0)
END_DECLARE_EVENT_TYPES()

class FindResultPage : public editor::TextWindow, public BookPage {
  DECLARE_CLASS(FindResultPage)

public:
  // Detailed event types of kFindResultPageEvent.
  enum {
    kLocalizeEvent = 1,

    // Find result page is destroyed.
    kDestroyEvent,
  };

public:
  explicit FindResultPage(editor::TextBuffer* buffer);
  virtual ~FindResultPage();

  bool Create(wxWindow* parent, wxWindowID id, bool hide = false);

  // Overriddens of BookPage:
  virtual bool Page_HasFocus() const override;
  virtual void Page_SetFocus() override;
  virtual void Page_Activate(bool active) override;
  virtual void Page_Close() override;
  virtual int Page_Type() const override;
  virtual wxString Page_Label() const override;
  virtual wxString Page_Description() const override;
  virtual int Page_Flags() const override;

  virtual void Page_EditMenu(wxMenu* edit_menu) override;
  virtual bool Page_EditMenuState(int menu_id) override;
  virtual bool Page_FileMenuState(int menu_id, wxString* text) override;
  virtual bool Page_OnMenu(int menu_id) override;
  virtual bool Page_Save() override;
  virtual bool Page_SaveAs() override;

protected:
  // Overriddens of editor::TextWindow:
  virtual void HandleTextLeftDClick(wxMouseEvent& evt) override;
  virtual void HandleTextRightUp(wxMouseEvent& evt) override;

  void PostEvent(int event_type);
};

}  // namespace jil

#define EVT_FIND_RESULT_PAGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(jil::kFindResultPageEvent, id, -1,\
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_FIND_RESULT_PAGE_H_
