@echo off
if not "%DevEnvDir%"=="" goto skipsetup

if exist "%VS140COMNTOOLS%..\..\VC\Bin\x86_amd64\vcvarsx86_amd64.bat" CALL "%VS140COMNTOOLS%..\..\VC\Bin\x86_amd64\vcvarsx86_amd64.bat"

:skipsetup

if "%1"=="" goto buildall
if "%2"=="" goto builddll

if not "%2"=="exe" set CFLAGS=%CFLAGS% /LD

if exist %1.rc rc %1.rc

if not exist %1.rc cl %CFLAGS% /Fe%1.%2 %1.cpp %ADDFILES% %3 %4 %5 %6 %7 %8 %9
if exist %1.res cl %CFLAGS% /Fe%1.%2 %1.cpp %1.res %ADDFILES% %3 %4 %5 %6 %7 %8 %9

if exist %1.exp del %1.exp
if exist %1.lib del %1.lib
if exist %1.obj del %1.obj
if exist %1.res del %1.res

if exist BlockingClient.obj del BlockingClient.obj
if exist EncodingUtils.obj del EncodingUtils.obj
if exist ScrobSubmitter.obj del ScrobSubmitter.obj

if not exist %1.%2 goto exitcmd
if "%1.%2" == "perb.exe" goto selfperb

if "%VER_STR%"=="" ..\..\..\..\buildutil\perb %1.%2
if not "%VER_STR%"=="" ..\..\..\..\buildutil\perb /v %VER_STR% %1.%2
editbin /release %1.%2
if not "%PREFIX2%"=="" copy %1.%2 %PREFIX2%%1.%2
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


set PREFIX=..\..\..\..\bin_x64\Plugins\Fittle\
set PREFIX2=
set CFLAGS=/GF /Gy /Ox /Os /EHsc
set ADDFILES=BlockingClient.cpp EncodingUtils.cpp ScrobSubmitter.cpp
set VER_STR=1706290X

call %0 last.fm dll

:exitcmd
