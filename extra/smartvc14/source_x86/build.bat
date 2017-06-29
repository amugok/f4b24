@echo off

if not "%VCINSTALLDIR%"=="" goto skipsetup
if not "%MSVCDir%"=="" goto skipsetup

if exist "%VS140COMNTOOLS%..\..\VC\Bin\vcvars32.bat" goto vc2015setup
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

:vc2015setup
call "%VS140COMNTOOLS%..\..\VC\Bin\vcvars32.bat"
goto skipsetup

:skipsetup

nasm -f win32 chkstk.nas
nasm -f win32 fltused.nas
nasm -f win32 ftol.nas
nasm -f win32 ftol2.nas
nasm -f win32 llmul.nas
nasm -f win32 lldiv.nas
nasm -f win32 memcpy.nas
nasm -f win32 memset.nas
nasm -f win32 ulldiv.nas

cl /nologo /c /Zl /O1 crt0init.c
cl /nologo /c /Zl /O1 crt0dll.c
cl /nologo /c /Zl /O1 crt0exe.c
cl /nologo /c /Zl /O1 memcmp.c
cl /nologo /c /Zl /O1 memmove.c

lib /NOLOGO /OUT:..\smartvc14_x86.lib crt0init.obj crt0dll.obj crt0exe.obj fltused.obj ftol.obj ftol2.obj chkstk.obj llmul.obj lldiv.obj memcmp.obj memcpy.obj memset.obj memmove.obj ulldiv.obj

del *.obj

