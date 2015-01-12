#ifndef JIL_FIND_RESULT_PAGE_H_
#define JIL_FIND_RESULT_PAGE_H_
#pragma once

// A text page to show the find result.

#include "app/text_page.h"

namespace jil {

class FindResultPage : public TextPage {
  DECLARE_CLASS(FindResultPage)

public:
  // Detailed event types of kFindResultPageEvent.
  enum EventType {
    kLocalizeEvent = 1,
  };

public:
  explicit FindResultPage(editor::TextBuffer* buffer);
  bool Create(wxWindow* parent, wxWindowID id, bool hide = false);
  virtual ~FindResultPage();

  // OVERRIDE of BookPage:
  virtual wxString Page_Label() const override;
  virtual wxString Page_Description() const override;
  virtual int Page_Flags() const override;

protected:
  virtual void HandleTextLeftDClick(wxMouseEvent& evt) override;

  virtual void FillRClickMenu(wxMenu& menu) override;
};

BEGIN_DECLARE_EVENT_TYPES()
// Check GetInt(), which returns enum FindResultPage::EventType, for the
// details.
DECLARE_EVENT_TYPE(kFindResultPageEvent, 0)
END_DECLARE_EVENT_TYPES()

}  // namespace jil

#define EVT_FIND_RESULT_PAGE(id, func)\
  DECLARE_EVENT_TABLE_ENTRY(jil::kFindResultPageEvent, id, -1, \
  wxCommandEventHandler(func), (wxObject*)NULL),

#endif  // JIL_FIND_RESULT_PAGE_H_
