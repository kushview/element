@echo off

set OPATH="%PATH%"
set PATH="%~dp0bin;%~dp0lib;%PATH%"
set LUA_PATH=%~dp0lua\?.lua
"%~dp0bin\element.exe" %*
set PATH="%OPATH%"
set OPATH=""
