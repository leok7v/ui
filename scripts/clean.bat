@echo off
rem Remove build artifacts. Runnable from anywhere (resolves repo root from %~dp0).
pushd "%~dp0.."
rmdir /s /q bin            2>nul
rmdir /s /q build          2>nul
rmdir /s /q lib            2>nul
rmdir /s /q msvc2022\.vs   2>nul
del   /q    tools\version\version.h 2>nul
popd
echo clean done
