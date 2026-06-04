; AnviTools Installer Script
; Требует: NSIS 3.x (https://nsis.sourceforge.io)

Unicode True

!define APP_NAME      "AnviTools"
!define APP_EXE       "AnviTools.exe"
!define APP_VERSION   "1.0"
!define PUBLISHER     "Anvicom"
!define INSTALL_DIR   "$PROGRAMFILES64\${APP_NAME}"
!define REG_KEY       "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "AnviTools_Setup.exe"
InstallDir "${INSTALL_DIR}"
InstallDirRegKey HKLM "${REG_KEY}" "InstallLocation"
RequestExecutionLevel admin
ShowInstDetails show
ShowUnInstDetails show

;--- Страницы установщика ---
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--- Установка ---
Section "Основные файлы" SEC_MAIN
    SetOutPath "$INSTDIR"

    File "AnviTools.exe"
    File "Qt5Core.dll"
    File "Qt5Gui.dll"
    File "Qt5Widgets.dll"
    File "Qt5SerialPort.dll"
    File "Qt5Svg.dll"
    File "libEGL.dll"
    File "libGLESv2.dll"
    File "D3Dcompiler_47.dll"
    File "harfbuzz.dll"
    File "freetype.dll"
    File "libpng16.dll"
    File "bz2.dll"
    File "brotlidec.dll"
    File "brotlicommon.dll"
    File "double-conversion.dll"
    File "pcre2-16.dll"

    SetOutPath "$INSTDIR\platforms"
    File "platforms\qwindows.dll"

    ; Ярлык на рабочем столе
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"

    ; Ярлык в меню Пуск
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Удалить ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"

    ; Запись в реестр (для "Установка и удаление программ")
    WriteRegStr   HKLM "${REG_KEY}" "DisplayName"      "${APP_NAME}"
    WriteRegStr   HKLM "${REG_KEY}" "DisplayVersion"   "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_KEY}" "Publisher"        "${PUBLISHER}"
    WriteRegStr   HKLM "${REG_KEY}" "InstallLocation"  "$INSTDIR"
    WriteRegStr   HKLM "${REG_KEY}" "UninstallString"  "$INSTDIR\uninstall.exe"
    WriteRegDWORD HKLM "${REG_KEY}" "NoModify"         1
    WriteRegDWORD HKLM "${REG_KEY}" "NoRepair"         1

    ; Создать деинсталлятор
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

;--- Удаление ---
Section "Uninstall"
    Delete "$INSTDIR\AnviTools.exe"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\Qt5SerialPort.dll"
    Delete "$INSTDIR\Qt5Svg.dll"
    Delete "$INSTDIR\libEGL.dll"
    Delete "$INSTDIR\libGLESv2.dll"
    Delete "$INSTDIR\D3Dcompiler_47.dll"
    Delete "$INSTDIR\harfbuzz.dll"
    Delete "$INSTDIR\freetype.dll"
    Delete "$INSTDIR\libpng16.dll"
    Delete "$INSTDIR\bz2.dll"
    Delete "$INSTDIR\brotlidec.dll"
    Delete "$INSTDIR\brotlicommon.dll"
    Delete "$INSTDIR\double-conversion.dll"
    Delete "$INSTDIR\pcre2-16.dll"
    Delete "$INSTDIR\platforms\qwindows.dll"
    Delete "$INSTDIR\uninstall.exe"

    RMDir "$INSTDIR\platforms"
    RMDir "$INSTDIR"

    Delete "$DESKTOP\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Удалить ${APP_NAME}.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"

    DeleteRegKey HKLM "${REG_KEY}"
SectionEnd
