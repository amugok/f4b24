@echo off

if "%1"=="" goto buildall
if "%2"=="" goto builddll

set PREFIXPERB=..\..\..\..\buildutil\
set CFLAGS=/MD /O1 /Gy /GR- /Gm- /EHs-c-

call %PREFIXPERB%vcsetup.bat
if not "XVCFLAGS"=="" set CFLAGS=%CFLAGS% %XVCFLAGS%
if not "%2"=="exe" set CFLAGS=%CFLAGS% /LD

if exist %1.rc rc %1.rc
if exist %1.res cl %CFLAGS% /Fe%1.%2 %1.cpp %1.res %3 %4 %5 %6 %7 %8 %9
if not exist %1.rc cl %CFLAGS% /Fe%1.%2 %1.cpp %3 %4 %5 %6 %7 %8 %9

if exist %1.exp del %1.exp
if exist %1.lib del %1.lib
if exist %1.obj del %1.obj
if exist %1.res del %1.res

if not exist %1.%2 goto exitcmd

if "%VER_STR%"=="" %PREFIXPERB%perb %1.%2
if not "%VER_STR%"=="" %PREFIXPERB%perb /v %VER_STR% %1.%2
editbin /release %1.%2
if not "%PREFIX%"=="" move %1.%2 %PREFIX%%1.%2

goto exitcmd

:builddll

call %0 %1 dll

goto exitcmd

:buildall

set PREFIX=..\..\..\..\bin\Plugins\Fittle\
set VER_STR=1104211A

call %0 minipane dll

set PREFIX=..\..\..\..\bin\Plugins\fgp\
set VER_STR=1104211U

call %0 minipaneu fgp

:exitcmd
