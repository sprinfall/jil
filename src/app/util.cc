#include "app/util.h"

#if defined (__WXMAC__)
#include <Cocoa/Cocoa.h>
#endif

#include "wx/stdpaths.h"
#include "wx/filename.h"
#include "wx/msgdlg.h"

#include "app/i18n_strings.h"

namespace jil {

namespace path {

wxString ExeDir() {
  wxFileName exe_fn(wxStandardPaths::Get().GetExecutablePath());
  return exe_fn.GetPath(
      wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, wxPATH_NATIVE);
}

wxString ResourceDir() {
#if defined (__WXMAC__)
  return wxStandardPaths::Get().GetResourcesDir();
#else
  return ExeDir();
#endif // __WXMAC__
}

} // namespace path

void ErrorMsg(const wxString& msg, wxWindow* parent) {
  wxMessageBox(msg, kTrError, wxOK | wxICON_ERROR | wxCENTRE);
}

void ExploreFile(const wxString& file_path) {
#if defined (__WXMSW__)
  wxString cmd = wxT("explorer /select, \"");
  cmd += file_path;
  cmd += wxT("\"");
  wxExecute(cmd);

#elif defined (__WXMAC__)
  NSString* nsstr = [[NSString alloc] initWithUTF8String: file_path.ToUTF8().data()];
  [[NSWorkspace sharedWorkspace] selectFile:(NSString*)nsstr inFileViewerRootedAtPath:nil];
  [nsstr release];
#endif
}

} // namespace jil
