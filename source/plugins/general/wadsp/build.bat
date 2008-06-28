@echo off
if not "%MSVCDir%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

if "%1"=="" goto buildall

if not exist %1.rc cl /Ox /Os /LD %1.cpp %2 %3 %4 %5 %6 %7 %8 %9
if exist %1.rc rc %1.rc
if exist %1.res cl /Ox /Os /LD %1.cpp %1.res %2 %3 %4 %5 %6 %7 %8 %9
if exist %1.exp del %1.exp
if exist %1.lib del %1.lib
if exist %1.obj del %1.obj
if exist %1.res del %1.res
if exist %1.dll perb %1.dll
if exist %1.dll editbin /release %1.dll
if exist %1.dll move %1.dll ..\..\..\..\bin\%1.dll

goto exitcmd

:buildall

call %0 wadsp /MD

:exitcmd
