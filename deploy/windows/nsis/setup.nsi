;
; Copyright (c) 2019 Alex Spataru <https://github.com/alex-spataru>
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
;

!include "MUI2.nsh"
!include "LogicLib.nsh"

!define APPNAME                      "RFID Manager"
!define UNIXNAME                     "RFID Manager"
!define COMPANYNAME                  "Matusica"
!define DESCRIPTION                  "RFID Tag Management Software"
!define VERSIONMAJOR                 0
!define VERSIONMINOR                 0
!define VERSIONBUILD                 1
!define ESTIMATED_SIZE               60000
!define MUI_ABORTWARNING
!define INSTALL_DIR                  "$PROGRAMFILES\${APPNAME}"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT      "Run ${APPNAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION  "RunApplication"
!define MUI_WELCOMEPAGE_TITLE        "Welcome to the ${APPNAME} installer!"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin"
        messageBox mb_iconstop "Administrator rights are required to install this software"
        setErrorLevel 740
        quit
${EndIf}
!macroend

Name "${APPNAME}"
ManifestDPIAware true
InstallDir "${INSTALL_DIR}"
RequestExecutionLevel admin
OutFile "${UNIXNAME}-${VERSIONMAJOR}.${VERSIONMINOR}${VERSIONBUILD}-setup.exe"

Function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
FunctionEnd

Section "${APPNAME} (required)" SecDummy
  SectionIn RO
  SetOutPath "${INSTALL_DIR}"
  File /r "${APPNAME}\*"
  DeleteRegKey HKCU "Software\${COMPANYNAME}\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}"

  WriteUninstaller "${INSTALL_DIR}\uninstall.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayName"     "${APPNAME}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "UninstallString" "${INSTALL_DIR}\uninstall.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "InstallLocation" "${INSTALL_DIR}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "Publisher"       "${COMPANYNAME}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayIcon"     "${INSTALL_DIR}\bin\${APPNAME}.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayVersion"   ${VERSIONMAJOR}.${VERSIONMINOR}${VERSIONBUILD}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "VersionMajor"     ${VERSIONMAJOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "VersionMinor"     ${VERSIONMINOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "NoModify"         1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "NoRepair"         1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "EstimatedSize"    ${ESTIMATED_SIZE}

SectionEnd

Section "Start Menu Shortcuts"
  CreateShortCut  "$SMPROGRAMS\${APPNAME}.lnk" "${INSTALL_DIR}\bin\${APPNAME}.exe" "" "${INSTALL_DIR}\bin\${APPNAME}.exe" 0
SectionEnd

Function RunApplication
  ExecShell "" "${INSTALL_DIR}\bin\${APPNAME}.exe"
FunctionEnd

Function un.onInit
	SetShellVarContext all
	MessageBox MB_OKCANCEL|MB_ICONQUESTION "Are you sure you want to uninstall ${APPNAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
FunctionEnd

Section "Uninstall"
  RMDir /r "${INSTALL_DIR}"
  RMDir /r "$SMPROGRAMS\${APPNAME}.lnk"
  DeleteRegKey HKCU "Software\${COMPANYNAME}\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}"
SectionEnd
