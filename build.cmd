@echo off
rem Build every dr_utils tool on Windows against DataReel 6.4.1.
rem Run from a Visual Studio x64 Native Tools prompt:
rem   set DATAREEL_DIR=C:\path\to\datareel-6.4.1
rem   build.cmd          build (64-bit release, 64BITCFG=1 FINAL=1)
rem   build.cmd clean    remove objects and programs
rem The DataReel library must be built first in %DATAREEL_DIR%\winslib.
rem cputest needs zlib: set ZLIB_DIR to a folder with zlib.h and zlib.lib.
setlocal
if "%DATAREEL_DIR%"=="" (
  echo Set DATAREEL_DIR to a DataReel 6.4.1 tree
  exit /b 2
)
set TOP=%~dp0
set FAIL=0
if not exist "%TOP%bin" mkdir "%TOP%bin"
for %%t in (file_hours file_purge filetimes internet-test pingscan dnsscan portscan netpscan arpscan getmac) do call :one tools %%t %1
for %%t in (cputest disktest memtest tcpserv tcptest udpserv udptest mcserv mctest) do call :one benchmarks %%t %1
exit /b %FAIL%

:one
rem %1 = folder, %2 = tool, %3 = clean
pushd "%TOP%%1\%2" || (set FAIL=1 & exit /b 0)
if "%3"=="clean" (
  nmake /nologo -f msvc.mak clean
  if exist "%TOP%bin\%2.exe" del "%TOP%bin\%2.exe"
  popd
  exit /b 0
)
echo == %2
nmake /nologo -f msvc.mak
if errorlevel 1 (
  echo ** %2 did not build
  set FAIL=1
) else (
  copy /y %2.exe "%TOP%bin\%2.exe" > nul
)
popd
exit /b 0
