@echo off

if not "%MSVCDir%"=="" goto skipsetup

if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT" CALL "%ProgramFiles%\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"

:skipsetup

perb /v "0807300U"  fittle.exe
editbin /release fittle.exe
