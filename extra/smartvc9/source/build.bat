@echo off
if not "%MSVCDir%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

nasm -f win32 chkstk.nas
nasm -f win32 ftol.nas
nasm -f win32 ftol2.nas
nasm -f win32 llmul.nas
nasm -f win32 lldiv.nas
nasm -f win32 ulldiv.nas
nasm -f win32 smartvc9.nas

cl /c /MD /Ogsy /Gs /Gy startup.cpp

lib smartvc9.obj startup.obj ftol.obj ftol2.obj chkstk.obj llmul.obj lldiv.obj ulldiv.obj

