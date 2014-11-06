#include "wx/filefn.h"
#include "wx/msgdlg.h"
#include "wx/filedlg.h"
#include "editor/text_buffer.h"

namespace jil {

bool SaveBuffer(editor::TextBuffer* buffer, wxWindow* parent) {
  editor::FileError error = buffer->SaveFile();
  if (error == editor::kNoError) {
    return true;
  }

  // Show error message.

  wxString msg = wxString::Format(_("Failed to save file! (%s)"),
                                  buffer->file_path_name());

  // Detailed error information.
  if (error == editor::kIOError) {
    msg += wxT(" ");
    msg += _("Is it writable?");
  } else if (error == editor::kEncodingError) {
    msg += wxT(" ");
    msg += _("File encoding issue.");
  }

  wxMessageBox(msg, _("Save File"), wxOK | wxCENTRE | wxICON_ERROR, parent);

  return false;
}

bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent) {
  wxString ext = buffer->file_name_object().GetExt();
  wxString file_path_name = wxSaveFileSelector(ext, wxEmptyString);
  if (file_path_name.empty()) {
    // Save As is canceled.
    return false;
  }

  if (wxFileExists(file_path_name)) {
    // Confirm replace.
    wxString msg = wxString::Format(
        _("The file already exists. Replace it? (%s)"),
        file_path_name);

    long flags = wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_EXCLAMATION;
    flags |= wxCENTRE;

    int confirm_result = wxMessageBox(msg, _("Save File As"), flags, parent);
    if (confirm_result == wxCANCEL) {
      // No replace.
      return false;
    }
  }

  // Switch buffer to the new saved file.
  buffer->set_file_path_name(file_path_name);
  buffer->Notify(editor::kFileNameChange);

  return SaveBuffer(buffer, parent);
}

}  // namespace jil
