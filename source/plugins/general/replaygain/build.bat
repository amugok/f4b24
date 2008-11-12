@echo off
if not "%MSVCDir%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

if "%1"=="" goto buildall
if "%2"=="" goto builddll

if not "%2"=="exe" set CFLAGS=%CFLAGS% /LD

if exist %1.rc rc %1.rc

if not exist %1.rc cl %CFLAGS% /Fe%1.%2 %1.cpp %3 %4 %5 %6 %7 %8 %9
if exist %1.res cl %CFLAGS% /Fe%1.%2 %1.cpp %1.res %3 %4 %5 %6 %7 %8 %9

if exist %1.exp del %1.exp
if exist %1.lib del %1.lib
if exist %1.obj del %1.obj
if exist %1.res del %1.res

if not exist %1.%2 goto exitcmd
if "%1.%2" == "perb.exe" goto selfperb

if "%VER_STR%"=="" perb %1.%2
if not "%VER_STR%"=="" perb /v %VER_STR% %1.%2
editbin /release %1.%2
if not "%PREFIX%"=="" move %1.%2 %PREFIX%%1.%2

goto exitcmd

:selfperb

copy %1.%2 .\perb_.exe
if "%VER_STR%"=="" .\perb_.exe %1.%2
if not "%VER_STR%"=="" .\perb_.exe /v %VER_STR% %1.%2
editbin /release %1.%2
del .\perb_.exe

goto exitcmd

:builddll

call %0 %1 dll

goto exitcmd

:buildall


set PREFIX=..\..\..\..\bin\Plugins\fgp\
set CFLAGS=/GF /Gy /Ox /Os /MD
set VER_STR=0811120_

call %0 replaygain fgp

:exitcmd
