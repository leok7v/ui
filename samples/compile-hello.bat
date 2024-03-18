@echo off
:: must be run from terminal using
:: Visual Studio 20xx Developer Command Prompt vXX.X.X
:: envirnment with cl.exe on the path
:: for example:
:: %comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat"
:: or
:: %comspec% /k "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

set CC_FLAGS=/GL /Gy -I..
set LINK_FLAGS=/LTCG /OPT:REF

cl.exe /Fe../bin/hello-windows.exe %CC_FLAGS% hello.c /link %LINK_FLAGS%
cl.exe /Fe../bin/hello-console.exe %CC_FLAGS% hello.c -DCONSOLE /link /subsystem:console %LINK_FLAGS%

start ..\bin\hello-windows.exe
..\bin\hello-console.exe foo bar

pause

