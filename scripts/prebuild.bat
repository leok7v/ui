@echo off
pushd ..
if exist bin\release\%1\version.exe (
    bin\release\%1\version.exe > inc/ut/version.h 2>nul
    goto version_done
)
if exist bin\debug\%1\version.exe (
    bin\debug\%1\version.exe > inc/ut/version.h 2>nul
    goto version_done
)
:version_done
popd
exit /b 0