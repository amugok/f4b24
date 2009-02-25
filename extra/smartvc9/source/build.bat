@echo off
if not "%MSVCDir%"=="" goto skipsetup
if not "%VCINSTALLDIR%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio 9.0\VC\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio 9.0\VC\Bin\VCVARS32.BAT"
rem if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

nasm -f win32 chkstk.nas
nasm -f win32 ftol.nas
nasm -f win32 ftol2.nas
nasm -f win32 llmul.nas
nasm -f win32 lldiv.nas
nasm -f win32 ulldiv.nas
nasm -f win32 smartvc9.nas

cl /nologo /c /Zl /Ogsy /Gs /Gy crt0init.c
cl /nologo /c /Zl /Ogsy /Gs /Gy crt0exe.c

lib /NOLOGO /OUT:..\smartvc9.lib smartvc9.obj crt0init.obj crt0exe.obj ftol.obj ftol2.obj chkstk.obj llmul.obj lldiv.obj ulldiv.obj

del *.obj

