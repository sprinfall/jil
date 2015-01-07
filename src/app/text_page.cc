#include "app/text_page.h"
#include "wx/menu.h"
#include "wx/msgdlg.h"
#include "wx/log.h"
#include "editor/text_buffer.h"
#include "editor/text_area.h"
#include "app/i18n_strings.h"
#include "app/id.h"
#include "app/save.h"

namespace jil {

using namespace editor;

IMPLEMENT_CLASS(TextPage, TextWindow)

TextPage::TextPage(TextBuffer* buffer)
    : TextWindow(buffer) {
}

TextPage::~TextPage() {
}

void TextPage::Page_Activate(bool active) {
  Show(active);
  SetFocus();
}

bool TextPage::Page_Close() {
  if (!buffer()->modified()) {
    Destroy();
    return true;
  }

  wxString confirm_msg;
  if (buffer()->new_created()) {
    confirm_msg = _("The file is untitled and changed, save it?");
  } else {
    confirm_msg = wxString::Format(
        _("The file (%s) has been changed, save it?"),
        Page_Label().c_str());
  }

  // Confirm save.
  int confirm_result = wxMessageBox(
      confirm_msg,
      _("Save File"),
      wxYES | wxNO | wxCANCEL | wxYES_DEFAULT | wxICON_EXCLAMATION | wxCENTRE);

  if (confirm_result == wxCANCEL) {
    // Don't close.
    return false;
  }

  if (confirm_result == wxNO) {
    // Don't save, close directly.
    Destroy();
    return true;
  }

  if (SaveBuffer()) {
    Destroy();
    return true;
  }

  return false;
}

wxString TextPage::Page_Label() const {
  return buffer()->file_name();
}

wxString TextPage::Page_Description() const {
  return buffer()->file_path_name();
}

int TextPage::Page_Flags() const {
  int flags = 0;
  if (buffer()->modified()) {
    flags |= kModified;
  }
  return flags;
}

bool TextPage::SaveBuffer() {
  bool saved = false;
  if (buffer()->new_created() || buffer()->read_only()) {
    saved = SaveBufferAs(buffer(), wxGetTopLevelParent(this));
  } else {
    saved = ::jil::SaveBuffer(buffer(), wxGetTopLevelParent(this));
  }
  return saved;
}

bool TextPage::OnTextMouse(wxMouseEvent& evt) {
  wxEventType evt_type = evt.GetEventType();

  if (evt_type == wxEVT_RIGHT_UP) {
    wxMenu menu;
    //menu.Append(wxID_ANY, _("Insert Snippet..."));
    //menu.AppendSeparator();
    //menu.Append(wxID_ANY, _("Go To Definition"));
    //menu.Append(wxID_ANY, _("Find References"));
    //menu.Append(wxID_ANY, _("Switch Between Header && Source"));
    //menu.AppendSeparator();
    menu.Append(ID_MENU_EDIT_CUT, _("Cut"));
    menu.Append(ID_MENU_EDIT_COPY, _("Copy"));
    menu.Append(ID_MENU_EDIT_PASTE, _("Paste"));

    wxPoint pos = text_area()->ClientToScreen(evt.GetPosition());
    pos = ScreenToClient(pos);

    PopupMenu(&menu, pos);

    return true;
  }

  return TextWindow::OnTextMouse(evt);
}

void TextPage::PostEvent(wxEventType evt_type) {
  wxCommandEvent evt(evt_type, GetId());
  evt.SetEventObject(this);
  GetParent()->GetEventHandler()->AddPendingEvent(evt);
}


TextPage* AsTextPage(BookPage* page) {
  if (page == NULL) {
    return NULL;
  }
  return wxDynamicCast(page->Page_Window(), TextPage);
}

std::vector<TextPage*> BookTextPages(BookCtrl* book) {
  std::vector<TextPage*> text_pages;
  std::vector<BookPage*> pages = book->Pages();
  for (size_t i = 0; i < pages.size(); ++i) {
    TextPage* text_page = AsTextPage(pages[i]);
    if (text_page != NULL) {
      text_pages.push_back(text_page);
    }
  }
  return text_pages;
}

}  // namespace jil
