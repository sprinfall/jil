#ifndef JIL_I18N_STRINGS_H_
#define JIL_I18N_STRINGS_H_
#pragma once

#include "wx/intl.h"
#include "wx/string.h"
#include "app/compile_config.h"

// Encoding names.
#define TR_ENCODING_UTF8             wxT("UTF-8")
#define TR_ENCODING_UTF8_BOM         wxT("UTF-8 BOM")
#define TR_ENCODING_UTF16_BE         wxT("UTF-16 BE")
#define TR_ENCODING_UTF16_LE         wxT("UTF-16 LE")
#define TR_ENCODING_GB18030          _("Chinese (GB18030)")
#define TR_ENCODING_BIG5             _("Chinese (Big5)")
#define TR_ENCODING_SHIFT_JIS        _("Japanese (Shift-JIS)")
#define TR_ENCODING_EUC_JP           _("Japanese (EUC-JP)")
#define TR_ENCODING_KOI8_R           _("Cyrillic (KOI8-R)")
#define TR_ENCODING_ISO_8859_1       _("Western (ISO 8859-1)")
#define TR_ENCODING_ISO_8859_2       _("Central European (ISO 8859-2)")
#define TR_ENCODING_ISO_8859_5       _("Cyrillic (ISO 8859-5)")
#define TR_ENCODING_ISO_8859_7       _("Greek (ISO 8859-7)")
#define TR_ENCODING_TIS_620          _("Thai (TIS 620)")
#define TR_ENCODING_WINDOWS_1250     _("Central European (Windows 1250)")
#define TR_ENCODING_WINDOWS_1251     _("Cyrillic (Windows 1251)")
#define TR_ENCODING_WINDOWS_1253     _("Greek (Windows 1253)")
#define TR_ENCODING_MAC_CYRILLIC     _("Cyrillic (Mac)")

// Plain text file type name.
#define kTrPlainText          _("Plain Text")

#define kTrError              _("Error")
#define kTrWarning            _("Warning")

#define kTrComingSoon         _("Coming soon...")
#define kTrFailedToLoad       _("Failed to load:")

#define kTrOpenFile           _("Open File")
#define kTrFailedToOpenFile   _("Failed to open file! (%s)")

#if defined (__WXMSW__)
#define kTrOptions            _("Options")
#else
#define kTrOptions            _("Preferences")
#endif  // __WXMSW__

#define kTrLine               _("Line")
#define kTrColumn             _("Column")

#define kTrOthers             _("Others")

// Page titles.
#define kTrPageFindResult     _("Find Result")
#define kTrPageUntitled       _("Untitled")

// Menu labels.

#if !defined (__WXOSX__)
// About menu will be placed in App menu on Mac while in Help menu on other platforms.
#  define kTrMenuAbout        _("&About Jil Text")
#endif  // !defined (__WXOSX__)

#define kTrMenuFile           _("&File")
#define kTrMenuEdit           _("&Edit")
#define kTrMenuView           _("&View")
//#define kTrMenuTools          _("&Tools")
#define kTrMenuPrefs          _("&Preferences")
#define kTrMenuHelp           _("&Help")

#define kTrFileNew            _("&New")
#if JIL_MULTIPLE_WINDOW
#  define kTrFileNewFrame       _("New &Window")
#endif  // JIL_MULTIPLE_WINDOW
#define kTrFileOpen           _("&Open")
#define kTrFileClose          _("&Close")
#define kTrFileCloseAll       _("Close A&ll")
#define kTrFileCloseFormat    _("&Close %s")
#define kTrFileSave           _("&Save")
#define kTrFileSaveFormat     _("&Save %s")
#define kTrFileSaveAs         _("Save &As...")
#define kTrFileSaveAsFormat   _("Save %s &As...")
#define kTrFileSaveAll        _("Save A&ll")
#define kTrFileRecentFiles    _("Recent &Files")

#if !defined (__WXOSX__)
#  define kTrFileExit         _("E&xit")
#endif  // !defined (__WXOSX__)

#define kTrEditUndo           _("&Undo")
#define kTrEditRedo           _("&Redo")
#define kTrEditCut            _("Cu&t")
#define kTrEditCopy           _("&Copy")
#define kTrEditPaste          _("&Paste")
#define kTrEditIndent         _("&Indent")
#define kTrEditIncreaseIndent _("&Increase Indent")
#define kTrEditDecreaseIndent _("&Decrease Indent")
#define kTrEditAutoIndent     _("&Auto Indent")
#define kTrEditComment        _("&Comment")
#define kTrEditUncomment      _("&Uncomment")
#define kTrEditFind           _("&Find")
#define kTrEditReplace        _("Rep&lace")
#define kTrEditGoTo           _("&Go To...")

#define kTrViewWrap           _("&Wrap")
#define kTrViewShowNumber     _("Show &Line Number")
#define kTrViewShowSpace      _("Show White &Space")
#define kTrViewFullScreen     _("&Full Screen")


// For find result page.
#define kTrEditClearAll       _("C&lear All")
#define kTrEditGoToLocation   _("&Go To Location")

#define kTrPrefsGlobal        _("&Global")
#define kTrPrefsEditor        _("&Syntax Specific")
#define kTrToolsTheme         _("&Theme")

#define kTrHelpViewOnline     _("&View Online Help")

// Right click floating menu labels.

#define kTrRClickCopy               _("Copy")
#define kTrRClickPaste              _("Paste")
#define kTrRClickCut                _("Cut")
#define kTrRClickSave               _("Save")
#define kTrRClickClose              _("Close")
#define kTrRClickCloseAll           _("Close All")
#define kTrRClickCloseAllButThis    _("Close All But This")
#define kTrRClickCopyFilePath       _("Copy File Path")
#ifdef __WXMAC__
#define kTrRClickOpenFolder         _("View In Finder")
#else
#define kTrRClickOpenFolder         _("Open Containing Folder")
#endif
#define kTrRClickNewFile            _("New File")

#define kTrRClickClearAll           _("Clear All")
#define kTrRClickGoToLocation       _("Go To Location")

#endif  // JIL_I18N_STRINGS_H_
