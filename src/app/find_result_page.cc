#include "app/find_result_page.h"
#include "editor/text_buffer.h"

namespace jil {

DEFINE_EVENT_TYPE(kFindResultPageEvent);

IMPLEMENT_CLASS(FindResultPage, TextPage);

FindResultPage::FindResultPage(editor::TextBuffer* buffer)
    : TextPage(buffer) {
  allow_text_change_ = false;
}

bool FindResultPage::Create(wxWindow* parent, wxWindowID id, bool hide) {
  if (!TextPage::Create(parent, id, hide)) {
    return false;
  }
  return true;
}

FindResultPage::~FindResultPage() {
}

wxString FindResultPage::Page_Label() const {
  return _("Find Result");
}

wxString FindResultPage::Page_Description() const {
  return wxEmptyString;
}

int FindResultPage::Page_Flags() const {
  return 0;
}

// Double-click goes to the line of this result.
void FindResultPage::HandleTextLeftDClick(wxMouseEvent& evt) {
  using namespace editor;

  Coord ln = CalcCaretPoint(evt.GetPosition(), false).y;
  if (ln > buffer_->LineCount()) {
    return;  // Click on the blank area.
  }

  wxCommandEvent fr_evt(kFindResultPageEvent, GetId());
  fr_evt.SetEventObject(this);
  fr_evt.SetInt(kLocalizeEvent);
  GetParent()->GetEventHandler()->AddPendingEvent(fr_evt);
}

}  // namespace jil
