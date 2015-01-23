#include "wx/filefn.h"
#include "wx/msgdlg.h"
#include "wx/filedlg.h"
#include "editor/text_buffer.h"

#define kTrSaveFile           _("Save File")
#define kTrSaveFileAs         _("Save File As")

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

  wxMessageBox(msg, kTrSaveFile, wxOK | wxCENTRE | wxICON_ERROR, parent);

  return false;
}

bool SaveBufferAs(editor::TextBuffer* buffer, wxWindow* parent) {
  wxFileDialog file_dialog(NULL,
                           kTrSaveFileAs,
                           wxEmptyString,
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

  if (wxFileExists(file_path)) {
    // Shouldn't be here since flag wxFD_OVERWRITE_PROMPT is used.
    // But just check it again.
    wxString msg = wxString::Format(
        _("The file already exists. Replace it? (%s)"),
        file_path);

    long flags = wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_EXCLAMATION;
    flags |= wxCENTRE;

    int confirm_result = wxMessageBox(msg, kTrSaveFileAs, flags, parent);
    if (confirm_result == wxCANCEL) {
      return false;
    }
  }

  // Switch buffer to the new saved file.
  buffer->set_file_path_name(file_path);

  buffer->Notify(editor::kFileNameChange);

  return SaveBuffer(buffer, parent);
}

}  // namespace jil
