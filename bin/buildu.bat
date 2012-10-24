@echo off

if not "%VCINSTALLDIR%"=="" goto skipsetup
if not "%MSVCDir%"=="" goto skipsetup

if exist "%VS100COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2010setup
if exist "%VS90COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2008setup
if exist "%VS80COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2005setup
if exist "%VS71COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2003setup
if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" goto vc6setup

goto skipsetup

:vc6setup
CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
goto skipsetup

:vc2003setup
CALL "%VS71COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:vc2005setup
CALL "%VS80COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:vc2008setup
CALL "%VS90COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:vc2010setup
CALL "%VS100COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup


:skipsetup

..\buildutil\perb /v "1210240U"  fittle.exe
editbin /release fittle.exe

copy Fittle.exe.manifest.txt Fittle.exe.manifest
