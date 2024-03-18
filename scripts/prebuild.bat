@echo off
pushd ..
call scripts\version.bat > samples/version.h 2>nul
popd
exit /b 0