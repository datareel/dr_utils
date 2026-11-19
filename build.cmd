@echo off
rem Build every dr_utils tool on Windows against DataReel 6.4.1.
rem Run from a Visual Studio x64 Native Tools prompt:
rem   set DATAREEL_DIR=C:\path\to\datareel-6.4.1
rem   build.cmd          build (64-bit release, 64BITCFG=1 FINAL=1)
rem   build.cmd clean    remove objects and programs
rem The DataReel library must be built first in %DATAREEL_DIR%\winslib.
setlocal
if "%DATAREEL_DIR%"=="" (
  echo Set DATAREEL_DIR to a DataReel 6.4.1 tree
  exit /b 2
)
set TOP=%~dp0
set FAIL=0
if not exist "%TOP%bin" mkdir "%TOP%bin"
for %%t in (file_hours file_purge filetimes internet-test pingscan dnsscan portscan netpscan arpscan getmac) do call :one %%t %1
exit /b %FAIL%

:one
pushd "%TOP%tools\%1" || (set FAIL=1 & exit /b 0)
if "%2"=="clean" (
  nmake /nologo -f msvc.mak clean
  if exist "%TOP%bin\%1.exe" del "%TOP%bin\%1.exe"
  popd
  exit /b 0
)
echo == %1
nmake /nologo -f msvc.mak
if errorlevel 1 (
  echo ** %1 did not build
  set FAIL=1
) else (
  copy /y %1.exe "%TOP%bin\%1.exe" > nul
)
popd
exit /b 0
