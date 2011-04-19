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

nasm -f win32 chkstk.nas
nasm -f win32 ftol.nas
nasm -f win32 ftol2.nas
nasm -f win32 llmul.nas
nasm -f win32 lldiv.nas
nasm -f win32 ulldiv.nas
nasm -f win32 smartvc9.nas

cl /nologo /c /Zl /O1 crt0init.c
cl /nologo /c /Zl /O1 crt0exe.c

lib /NOLOGO /OUT:..\smartvc9.lib smartvc9.obj crt0init.obj crt0exe.obj ftol.obj ftol2.obj chkstk.obj llmul.obj lldiv.obj ulldiv.obj

del *.obj

