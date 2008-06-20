@echo off
if not "%MSVCDir%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

if "%1"=="" goto buildall

cl /Ogisyb1 /LD %1.cpp %2 %3 %4 %5 %6 %7 %8 %9
if exist %1.exp del %1.exp
if exist %1.lib del %1.lib
if exist %1.obj del %1.obj
if exist %1.dll perb %1.dll
if exist %1.dll editbin /release %1.dll
if exist %1.dll move %1.dll ..\..\..\bin\%1.fap

goto exitcmd

:buildall

call %0 arj /MD
call %0 cab /MD
call %0 cue /MD
call %0 gca /GX /link /NODEFAULTLIB:libcmt.lib msvcrt.lib
call %0 lha /MD
call %0 rar /MD
call %0 tar /MD
call %0 zip /MD

call %0 arju /MD
call %0 cabu /MD
call %0 cueu /MD
call %0 lhau /MD
call %0 gcau /GX /link /NODEFAULTLIB:libcmt.lib msvcrt.lib
call %0 raru /MD
call %0 taru /MD
call %0 zipu /MD

:exitcmd
