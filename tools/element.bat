@echo off

set OPATH="%PATH%"
set PATH="%~dp0bin;%~dp0lib;%PATH%"
"%~dp0bin\element.exe" %*
set PATH="%OPATH%"
set OPATH=""
