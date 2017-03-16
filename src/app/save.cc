#include "app/save.h"

#include "wx/filedlg.h"
#include "wx/filefn.h"
#include "wx/msgdlg.h"
#include "wx/richmsgdlg.h"

#include "editor/ft_plugin.h"
#include "editor/text_buffer.h"

#include "app/app.h"
#include "app/text_page.h"

#define kTrSaveFile _("Save File")
#define kTrSaveFileAs _("Save File As")

#define kTrSaveModifiedMsgFormat _("The file '%s' has been changed. Save it?")
#define kTrSaveNewCreatedMsg _("The file is untitled and changed. Save it?")
#define kTrApplyToAll _("Apply to all")

namespace jil {

////////////////////////////////////////////////////////////////////////////////

typedef wxString(*GetSaveMsgFunc)(const TextPage*);

static wxString GetModifiedSaveMsg(const TextPage* text_page) {
  return wxString::Format(kTrSaveModifiedMsgFormat, text_page->Page_Label());
}

static wxString GetNewCreatedSaveMsg(const TextPage* WXUNUSED(text_page)) {
  return kTrSaveNewCreatedMsg;
}

////////////////////////////////////////////////////////////////////////////////

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent) {
  editor::FileError error = buffer->SaveFile();
  if (error == editor::kNoError) {
    return true;
  }

  // Show error message.

  wxString msg = wxString::Format(_("Can't save '%s'."), buffer->file_path_name());

  // Detailed error information.
  if (error == editor::kIOError) {
    msg += wxT("\n");
    msg += _("Please make sure the file is not read-only.");
  } else if (error == editor::kEncodingError) {
    msg += wxT(" ");
    msg += _("Please save with another file encoding.");
  }

  wxMessageBox(msg, kTrSaveFile, wxOK|wxCENTRE|wxICON_ERROR, parent);

  return false;
}

bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent) {
  wxString default_dir = buffer->file_path();

  wxFileDialog file_dialog(parent,
                           kTrSaveFileAs,
                           default_dir,
                           buffer->file_name(),
                           wxFileSelectorDefaultWildcardStr,  // "*.*", all files.
                           wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (file_dialog.ShowModal() == wxID_CANCEL) {
    return false;
  }

  wxString file_path = file_dialog.GetPath();
  if (file_path.empty()) {
    // Shouldn't be here. wxFileDialog should have checked it.
    return false;
  }

  // wxFileDialog should confirm the overwrite since we give the flag wxFD_OVERWRITE_PROMPT.
#if 0
  if (wxFileExists(file_path)) {
    // Shouldn't be here since flag wxFD_OVERWRITE_PROMPT is used.
    // But just check it again.
    wxString msg = wxString::Format(_("The file '%s' already exists. Replace it?"), file_path);

    long style = wxOK|wxCANCEL|wxCANCEL_DEFAULT|wxICON_EXCLAMATION|wxCENTRE;
    int confirm_result = wxMessageBox(msg, kTrSaveFileAs, style, parent);
    if (confirm_result == wxCANCEL) {
      return false;
    }
  }
#endif  // 0

  // Backup the old file type id.
  wxString old_ft_id = buffer->ft_plugin()->id();

  // Set new file path.
  buffer->set_file_path_name(file_path);

  // Get the new file type from the new file ext.
  const editor::FileType& ft = wxGetApp().FileTypeFromFileName(buffer->file_name_object());

  // Update the ft plugin if the file type is changed.
  if (ft.id != old_ft_id) {
    buffer->SetFtPlugin(wxGetApp().GetFtPlugin(ft));
  }

  buffer->Notify(editor::kFileNameChange);

  return SaveBuffer(buffer, parent);
}


int ConfirmSave(TextPage* text_page) {
  editor::TextBuffer* buffer = text_page->buffer();

  assert(buffer->modified());

  wxString msg;
  if (buffer->new_created()) {
    msg = kTrSaveNewCreatedMsg;
  } else {
    wxString msg_format = kTrSaveModifiedMsgFormat;
    msg = wxString::Format(msg_format, text_page->Page_Label());
  }

  long style = wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_EXCLAMATION | wxCENTRE;
  return wxMessageBox(msg, kTrSaveFile, style);
}

// NOTE:
// wxRichMessageDialog returns wxID_YES instead of wxYES, wxID_NO
// instead of wxNO, etc.
static bool SaveWithConfirm(wxWindow* parent,
                            const std::vector<TextPage*>& text_pages,
                            GetSaveMsgFunc get_save_msg) {
  assert(!text_pages.empty());

  long style = wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_EXCLAMATION | wxCENTRE;

  bool confirm = true;

  std::vector<TextPage*>::const_iterator it = text_pages.begin();
  for (; it != text_pages.end(); ++it) {
    TextPage* text_page = *it;

    int code = wxID_YES;

    // Show confirm message.
    if (confirm) {
      wxString msg = get_save_msg(text_page);

      // Show "Apply to all" check box if there are multiple text pages to save.
      bool show_checkbox = std::distance(it, text_pages.end()) > 1;

      wxRichMessageDialog dialog(parent, msg, kTrSaveFile, style);

      if (show_checkbox) {
        dialog.ShowCheckBox(kTrApplyToAll);
      }

      code = dialog.ShowModal();

      if (show_checkbox && dialog.IsCheckBoxChecked()) {
        confirm = false;  // Don't confirm next time.
      }
    }

    if (code == wxID_YES) {
      if (!Save(text_page->buffer(), parent)) {
        return false;
      }
    } else if (code == wxID_NO) {
      if (!confirm) {
        return true;
      }
    } else {  // wxID_CANCEL
      return false;
    }
  }

  return true;
}

bool SaveModifiedWithConfirm(wxWindow* parent,
                             const std::vector<TextPage*>& text_pages) {
  return SaveWithConfirm(parent, text_pages, GetModifiedSaveMsg);
}

bool SaveNewCreatedWithConfirm(wxWindow* parent,
                               const std::vector<TextPage*>& text_pages) {
  return SaveWithConfirm(parent, text_pages, GetNewCreatedSaveMsg);
}

bool Save(editor::TextBuffer* buffer, wxWindow* parent) {
  bool saved = false;
  if (buffer->new_created() || buffer->read_only()) {
    saved = SaveBufferAs(buffer, parent);
  } else {
    saved = SaveBuffer(buffer, parent);
  }
  return saved;
}

}  // namespace jil
