@echo off
rem Reproducible native-Windows build of adlibgold.sys (call/0007, call/0009), the
rem second host of the dual-hosted lane. It mirrors build.sh command-for-command and
rem uses the same C:\ paths the Wine build uses (C:\tc, C:\drv, C:\temp), so the VC6
rem toolchain sees identical inputs and emits a byte-identical artifact. No network at
rem build time; the pinned deps-bundle is staged from %BUNDLE%.
rem   set BUNDLE=<path-to-bundle.tar.gz> && build.bat
setlocal
set HERE=%~dp0
if "%BUNDLE%"=="" ( echo set BUNDLE to the deps-bundle tar.gz & exit /b 1 )

if not exist C:\drv  mkdir C:\drv
if not exist C:\temp mkdir C:\temp

rem Stage the pinned toolchain (offline) and the driver sources, exactly as build.sh.
tar -C C:\ -xzf "%BUNDLE%" || exit /b 1
copy /y "%HERE%*.cpp" C:\drv\ >nul
copy /y "%HERE%*.h"   C:\drv\ >nul
copy /y C:\tc\src\stdunk.cpp C:\drv\ >nul

set DEFS=/DWIN32=100 /D_WIN32_WINNT=0x0500 /DWINVER=0x0500 /D_WIN32_IE=0x0400 /D_X86_=1 /Di386=1 /DSTD_CALL /DCONDITION_HANDLING=1 /DNT_UP=1 /DWINNT=1 /DDEVL=1 /DNDEBUG /DUNICODE /D_UNICODE
set TMP=C:\temp
set TEMP=C:\temp
set INCLUDE=C:\tc\ddkinc

for %%s in (common adapter algtopo algwave fmsynth midi stdunk) do (
    C:\tc\bin\CL.EXE /nologo /c /Zel /Gz /Gy /Gm- /GF /W3 /Od %DEFS% /FoC:\drv\%%s.obj C:\drv\%%s.cpp || exit /b 1
)

C:\tc\bin\LIB.EXE /nologo /out:C:\tc\ddklib\stdunk.lib C:\drv\stdunk.obj || exit /b 1

set LIB=C:\tc\ddklib;C:\tc\vclib
C:\tc\bin\LINK.EXE /nologo /out:C:\drv\adlibgold.sys /machine:ix86 /subsystem:native,5.00 /driver /base:0x10000 /align:0x80 /entry:DriverEntry@8 /merge:_PAGE=PAGE /merge:_TEXT=.text /merge:.rdata=.text /nodefaultlib /release /incremental:no C:\drv\common.obj C:\drv\adapter.obj C:\drv\algtopo.obj C:\drv\algwave.obj C:\drv\fmsynth.obj C:\drv\midi.obj portcls.lib libcntpr.lib ntoskrnl.lib hal.lib stdunk.lib || exit /b 1

python "%HERE%pe_normalize.py" C:\drv\adlibgold.sys || exit /b 1
copy /y C:\drv\adlibgold.sys "%HERE%adlibgold.sys" >nul
for /f "usebackq" %%h in (`certutil -hashfile "%HERE%adlibgold.sys" SHA256 ^| findstr /r "^[0-9a-f]"`) do echo adlibgold.sys sha256: %%h
