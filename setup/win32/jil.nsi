!include LogicLib.nsh

; Use the new visual style on XP and later.
XPStyle on

; Setup exe name, see OutFile.
!define SETUP_NAME "JilText"

!ifndef PRODUCT_VERSION
!define PRODUCT_VERSION "1.0"
!endif

; Product name displayed to the user.
!define PRODUCT_NAME "Jil Text"

!define EXE_NAME "jil.exe"

!define UNINST_EXE_NAME "uninst.exe"

; Final setup file name.
OutFile "Setup${SETUP_NAME}_${PRODUCT_VERSION}.exe"

; The icon of the setup file.
Icon "setup.ico"

; Name the title of the installer.
Name "${PRODUCT_NAME}"

;-------------------------------------------------------------------------------
; Callback functions.

Function .onInit

  StrCpy $INSTDIR "$PROGRAMFILES\${PRODUCT_NAME}"

FunctionEnd

;-------------------------------------------------------------------------------
; Pages

; Page license

; Page components

; Page directory

Page instfiles

Section "Jil Text"

  SetOutPath $INSTDIR\ftplugin
  File /r "..\..\data\jilfiles\ftplugin\*"

  SetOutPath $INSTDIR\theme
  File /r "..\..\data\jilfiles\theme\*"

  SetOutPath $INSTDIR

  File "..\..\data\jilfiles\binding.cfg"
  File "..\..\data\jilfiles\file_types.cfg"
  File "..\..\data\jilfiles\options.cfg"
  File "..\..\data\jilfiles\status_fields.cfg"

  File "${EXE_NAME}"

  WriteUninstaller "$INSTDIR\${UNINST_EXE_NAME}"

  SetShellVarContext current

  ; Shortcuts

  ; Start menu
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"

  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE_NAME}"
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\${UNINST_EXE_NAME}"

  ; Desktop
  CreateShortcut "$DESKTOP\${PRODUCT_NAME}.lnk" "$INSTDIR\${EXE_NAME}"

SectionEnd

; Uninstall section.
; The name must be "Uninstall".
Section "Uninstall"
  ; Don't use "RMDir /r $INSTDIR", it's too dangerious!

  RMDir /r "$INSTDIR\ftplugin"
  RMDir /r "$INSTDIR\theme"

  Delete "$INSTDIR\binding.cfg"
  Delete "$INSTDIR\file_types.cfg"
  Delete "$INSTDIR\options.cfg"
  Delete "$INSTDIR\status_fields.cfg"

  Delete "$INSTDIR\${EXE_NAME}"
  Delete "$INSTDIR\${UNINST_EXE_NAME}"  ; Delete self.

  ; Shortcuts
  Delete "$DESKTOP\${PRODUCT_NAME}.lnk"
  RMDir /r "$SMPROGRAMS\${PRODUCT_NAME}"

  ; App data
  RMDir /r "$APPDATA\${PRODUCT_NAME}"

SectionEnd

; UninstPage uninstConfirm

; UninstPage instfiles

