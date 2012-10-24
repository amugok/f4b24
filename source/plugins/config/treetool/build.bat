@echo off

if not "%VCINSTALLDIR%"=="" goto skipsetup
if not "%MSVCDir%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" goto vc6setup
if exist "%VS100COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2010setup
if exist "%VS90COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2008setup
if exist "%VS80COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2005setup
if exist "%VS71COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2003setup

goto skipsetup

:vc6setup
call "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
goto skipsetup

:vc2003setup
call "%VS71COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:vc2005setup
call "%VS80COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:vc2008setup
call "%VS90COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:vc2010setup
call "%VS100COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

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

if "%VER_STR%"=="" %PREFIXPERB%perb %1.%2
if not "%VER_STR%"=="" %PREFIXPERB%perb /v %VER_STR% %1.%2
editbin /release %1.%2
if not "%PREFIX%"=="" move %1.%2 %PREFIX%%1.%2

goto exitcmd

:builddll

call %0 %1 dll

goto exitcmd

:buildall


set PREFIXPERB=..\..\..\..\buildutil\
set CFLAGS=/GF /Gy /Ox /Os /GS- /GR- /Gm- /EHs-c- /MD

set PREFIX=..\..\..\..\bin\Plugins\fcp\
set VER_STR=1210240_

call %0 treetool fcp

:exitcmd
