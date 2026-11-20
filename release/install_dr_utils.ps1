# -----------------------------------------------------------
# PowerShell Script: install_dr_utils.ps1
# Produced By: DataReel Software Development Team
# File Creation Date: 11/20/2026
# Date Last Modified: 11/20/2026
# -----------------------------------------------------------
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library. If not, see
# <https://www.gnu.org/licenses/>.
# -----------------------------------------------------------
<#
.SYNOPSIS
Installs the dr_utils programs from the Windows binary package
(dr_utils_<version>_windows_binaries.zip, 64-bit, or
dr_utils_<version>_windows_32bit_binaries.zip, 32-bit).

.DESCRIPTION
Dry run by default: it shows what it would copy. Run with -Install to
copy. Nothing is removed; programs already in the target folder are kept
as name.backup-YYYYMMDD-HHMMSS. The sha256 of every file is checked first.

The programs go to bin\ and the Python scripts (dr_plot.py, dr_report.py)
to scripts\ under the install folder, and bin\ is added to the PATH.
Windows has its own getmac.exe in System32, which comes first in the PATH,
so the dr_utils getmac is installed as dr_getmac.exe too.

  install_dr_utils.ps1 [-Install] [-System | -Prefix DIR] [-NoPath]
    -System   install in "%ProgramFiles%\DataReel\dr_utils" for every user
              and add it to the system PATH (run as an administrator)
    -Prefix   install in DIR (default: %LOCALAPPDATA%\DataReel\dr_utils,
              added to your own PATH)
    -NoPath   do not change the PATH

From cmd.exe:
  powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1
  powershell -ExecutionPolicy Bypass -File install_dr_utils.ps1 -Install

The reporting agent hook files (results to the DRSM dashboard) have their
own installer: hooks\windows\install_hooks.ps1, see docs.

Changes:
11/20/2026: First version, the same steps as install_fcrypt.sh.
#>
param(
  [switch]$Install,
  [switch]$System,
  [string]$Prefix = '',
  [switch]$NoPath
)
$ErrorActionPreference = 'Stop'
$pkg = Split-Path -Parent $MyInvocation.MyCommand.Path
$version = 'unknown'
if (Test-Path "$pkg\VERSION") { $version = (Get-Content "$pkg\VERSION" -TotalCount 1).Trim() }
if ($System -and $Prefix) { Write-Error 'Use -System or -Prefix, not both'; exit 1 }
if ($System) { $Prefix = Join-Path $env:ProgramFiles 'DataReel\dr_utils' }
elseif (-not $Prefix) { $Prefix = Join-Path $env:LOCALAPPDATA 'DataReel\dr_utils' }
$binDir = Join-Path $Prefix 'bin'
$scrDir = Join-Path $Prefix 'scripts'
$dry = if ($Install) { '' } else { 'DRY RUN: ' }

$arch = 'unknown'
if (Test-Path "$pkg\ARCH") { $arch = (Get-Content "$pkg\ARCH" -TotalCount 1).Trim() }
Write-Output "dr_utils $version, Windows $arch build"
if ($arch -eq 'x86' -and [Environment]::Is64BitOperatingSystem) {
  Write-Output 'Note: this is the 32-bit package on 64-bit Windows; the 64-bit package is the preferred one'
}
if ($arch -eq 'x64' -and -not [Environment]::Is64BitOperatingSystem) {
  Write-Error 'This is the 64-bit package and this Windows is 32-bit: use dr_utils_<version>_windows_32bit_binaries.zip'
  exit 1
}

# sha256 check of every file listed in sha256sum.txt (bin\ and scripts\)
$bad = 0
$files = @()
foreach ($line in Get-Content "$pkg\sha256sum.txt") {
  if ($line -notmatch '^([0-9a-fA-F]{64}) [ *](.+)$') { continue }
  $want = $Matches[1].ToLower(); $rel = $Matches[2] -replace '/', '\'
  $path = Join-Path $pkg $rel
  if (-not (Test-Path $path)) { Write-Output "missing: $rel"; $bad++; continue }
  $got = (Get-FileHash -Algorithm SHA256 $path).Hash.ToLower()
  if ($got -ne $want) { Write-Output "sha256 mismatch: $rel"; $bad++; continue }
  $files += $rel
}
if ($bad -gt 0 -or $files.Count -eq 0) { Write-Error 'The sha256 check failed, nothing was installed'; exit 1 }
Write-Output "sha256 check of $($files.Count) files: OK"

# One program must run here (the C runtime is linked in, no DLLs needed)
$out = & "$pkg\bin\cputest.exe" --version 2>&1 | Out-String
if ($out -notmatch "cputest version $([regex]::Escape($version))") {
  Write-Error "bin\cputest.exe does not run on this computer, nothing was installed: $out"
  exit 1
}

if ($Install -and $System) {
  $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
  if (-not $admin) { Write-Error 'Run -Install -System as an administrator'; exit 1 }
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$plan = @()
foreach ($rel in $files) {
  $dir = if ($rel -like 'bin\*') { $binDir } else { $scrDir }
  $name = Split-Path -Leaf $rel
  $plan += ,@((Join-Path $pkg $rel), (Join-Path $dir $name))
  if ($name -eq 'getmac.exe') { $plan += ,@((Join-Path $pkg $rel), (Join-Path $dir 'dr_getmac.exe')) }
}
foreach ($p in $plan) {
  if (Test-Path $p[1]) { Write-Output "${dry}keep $($p[1]) as $($p[1]).backup-$stamp" }
  Write-Output "${dry}install $($p[0]) -> $($p[1])"
}

$scope = if ($System) { 'Machine' } else { 'User' }
$path = [Environment]::GetEnvironmentVariable('Path', $scope)
$inPath = ($path -split ';' | Where-Object { $_.TrimEnd('\') -ieq $binDir.TrimEnd('\') }).Count -gt 0
if (-not $NoPath -and -not $inPath) { Write-Output "${dry}add $binDir to the $($scope.ToLower()) PATH" }

if (-not $Install) {
  Write-Output 'Dry run only, nothing was changed. Run with -Install to install.'
  exit 0
}

New-Item -ItemType Directory -Force -Path $binDir, $scrDir | Out-Null
foreach ($p in $plan) {
  if (Test-Path $p[1]) { Move-Item -Force $p[1] "$($p[1]).backup-$stamp" }
  # Copy to a new name, then rename, so a running copy is never overwritten
  Copy-Item -Force $p[0] "$($p[1]).new"
  Move-Item -Force "$($p[1]).new" $p[1]
}
if (-not $NoPath -and -not $inPath) {
  $new = if ($path) { $path.TrimEnd(';') + ';' + $binDir } else { $binDir }
  [Environment]::SetEnvironmentVariable('Path', $new, $scope)
  Write-Output "Added $binDir to the $($scope.ToLower()) PATH (open a new prompt to use it)"
}
Write-Output "Installed in ${Prefix}: $($plan.Count) programs and scripts"
& (Join-Path $binDir 'cputest.exe') --version 2>&1 | Select-Object -First 1
Write-Output 'Use dr_getmac (or the full path) for the dr_utils getmac: Windows has its own getmac.exe.'
Write-Output 'dr_plot.py and dr_report.py need Python 3 with matplotlib: py -m pip install matplotlib'
exit 0
