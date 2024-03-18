@echo off
pushd ..
call bin\Release\version.exe > build/version.h 2>nul
popd
exit /b 0