#include "wx/filefn.h"
#include "wx/msgdlg.h"
#include "wx/filedlg.h"
#include "editor/text_buffer.h"
#include "editor/ft_plugin.h"
#include "app/app.h"
#include "app/text_page.h"
#include "app/text_book.h"  // TODO

#define kTrSaveFile           _("Save File")
#define kTrSaveFileAs         _("Save File As")

namespace jil {

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent) {
  editor::FileError error = buffer->SaveFile();
  if (error == editor::kNoError) {
    return true;
  }

  // Show error message.

  wxString msg = wxString::Format(_("Failed to save file! (%s)"), buffer->file_path_name());

  // Detailed error information.
  if (error == editor::kIOError) {
    msg += wxT(" ");
    msg += _("Is it writable?");
  } else if (error == editor::kEncodingError) {
    msg += wxT(" ");
    msg += _("File encoding issue.");
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
    wxString msg = wxString::Format(_("The file already exists. Replace it? (%s)"), file_path);

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
  const editor::FileType& ft = wxGetApp().FileTypeFromExt(buffer->file_ext());

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
    msg = _("The file is untitled and changed, save it?");
  } else {
    msg = wxString::Format(_("The file (%s) has been changed, save it?"), text_page->GetLabel().c_str());
  }

  long style = wxYES | wxNO | wxCANCEL | wxYES_DEFAULT | wxICON_EXCLAMATION | wxCENTRE;
  return wxMessageBox(msg, _("Save File"), style);
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
