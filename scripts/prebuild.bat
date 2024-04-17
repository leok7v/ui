@echo off
pushd ..
if exist bin\release\version.exe (
    bin\release\version.exe > inc/ut/version.h 2>nul
    goto version_done
)
if exist bin\debug\version.exe (
    bin\debug\version.exe > inc/ut/version.h 2>nul
    goto version_done
)
:version_done
popd
exit /b 0