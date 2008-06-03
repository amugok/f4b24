@echo off
if not "%MSVCDir%"=="" goto skipsetup

if exist "C:\Program Files\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "C:\Program Files\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

if "%1"=="" goto buildall

cl /Ox /Os /LD %1.cpp %2 %3 %4 %5 %6 %7 %8 %9
if exist %1.exp del %1.exp
if exist %1.lib del %1.lib
if exist %1.obj del %1.obj
if exist %1.dll move %1.dll ..\..\bin\%1.dll

goto exitcmd

:buildall

call %0 taskvol /MD
call %0 xdelfile /MD
rc minipane.rc
if exist minipane.res call %0 minipane minipane.res /MD
if exist minipane.res del minipane.res

:exitcmd
