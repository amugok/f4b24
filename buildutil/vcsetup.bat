@echo off

if not "%VCINSTALLDIR%"=="" goto skipsetup
if not "%MSVCDir%"=="" goto skipsetup

if exist "%VS100COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2010setup
if exist "%VS100COMNTOOLS%vsvars32.bat" goto vs2010setup
if exist "%VS90COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2008setup
if exist "%VS90COMNTOOLS%vsvars32.bat" goto vs2008setup
if exist "%VS80COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2005setup
if exist "%VS80COMNTOOLS%vsvars32.bat" goto vs2005setup
if exist "%VS71COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2003setup
if exist "%VS71COMNTOOLS%vsvars32.bat" goto vs2003setup
if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" goto vc6setup

goto skipsetup

:vc6setup
call "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
set XVCFLAGS=
goto skipsetup

:vc2003setup
call "%VS71COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vs2003setup
call "%VS71COMNTOOLS%vsvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vc2005setup
call "%VS80COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vs2005setup
call "%VS80COMNTOOLS%vsvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vc2008setup
call "%VS90COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vs2008setup
call "%VS90COMNTOOLS%vsvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vc2010setup
call "%VS100COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:vs2010setup
call "%VS100COMNTOOLS%vsvars32.bat"
set XVCFLAGS=/GL /GS-
goto skipsetup

:skipsetup
