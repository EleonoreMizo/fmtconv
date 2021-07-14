
@cd /d "%~dp0"

@mkdir "releases"
@if exist "releases\fmtconv-rnew.zip" del "releases\fmtconv-rnew.zip"
@rmdir /s /q "reltmp"

@mkdir "reltmp"
@mkdir "reltmp\src"
@mkdir "reltmp\build"
@mkdir "reltmp\build\win"
@mkdir "reltmp\win32"
@mkdir "reltmp\win64"
@xcopy /I "doc"                                "reltmp\doc"
@xcopy /I "build\unix"                         "reltmp\build\unix"
@xcopy /I "build\unix\m4"                      "reltmp\build\unix\m4"
@xcopy /I "src\conc"                           "reltmp\src\conc"
@xcopy /I "src\ffft"                           "reltmp\src\ffft"
@xcopy /I "src\fmtc"                           "reltmp\src\fmtc"
@xcopy /I "src\fmtcl"                          "reltmp\src\fmtcl"
@xcopy /I "src\fstb"                           "reltmp\src\fstb"
@xcopy /I "src\vsutl"                          "reltmp\src\vsutl"
@copy     "src\*.cpp"                          "reltmp\src"
@copy     "src\*.h"                            "reltmp\src"
@copy     "src\*.hpp"                          "reltmp\src"
@copy     "build\win\*.sln"                    "reltmp\build\win"
@copy     "build\win\*.vcxproj"                "reltmp\build\win"
@copy     "build\win\*.vcxproj.filters"        "reltmp\build\win"
@copy     "build\win\ReleaseWin32\fmtconv.dll" "reltmp\win32"
@copy     "build\win\Releasex64\fmtconv.dll"   "reltmp\win64"
@copy     "*.md"                               "reltmp"
@copy     "COPYING"                            "reltmp"

@cd reltmp
@echo fmtconv - Format conversion tools for Vapoursynth. | "C:\Program Files (x86)\Infozip\zip.exe" -r -o -9 -z "..\releases\fmtconv-rnew.zip" "*.*"
@cd ..

@rmdir /s /q "reltmp"

@pause
