# -----------------------------------------------------------
# PowerShell Script: install_hooks.ps1
# Produced By: DataReel Software Development Team
# File Creation Date: 11/19/2026
# Date Last Modified: 11/19/2026
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
#
# Changes:
# 11/19/2026: First version.
# -----------------------------------------------------------
<#
.SYNOPSIS
  Installs dr_utils upload hooks on a computer with the DataReel
  Reporting Agent for Windows (3.3-beta2 or later).

.DESCRIPTION
  Copies hooks\uploads.d\<id>.ini to
  C:\ProgramData\DataReel\ReportingAgent\uploads.d\. After that a
  dr_utils program run with --submit hands its results to the agent
  (drra_svc.exe submit --hook <id>). A dry run unless -Apply is given.
  The DRSM manifests (hooks\drsm\uploads\<id>) go on the monitor host
  by hand, see hooks\README.md.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File hooks\windows\install_hooks.ps1 all
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File hooks\windows\install_hooks.ps1 cputest disktest -Apply
#>
param(
  [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments = $true)][string[]]$Hooks,
  [switch]$Apply
)
$ErrorActionPreference = 'Stop'
$iniDir = Join-Path (Split-Path -Parent $PSScriptRoot) 'uploads.d'
$uploadsD = Join-Path $env:ProgramData 'DataReel\ReportingAgent\uploads.d'

$ids = @()
foreach ($h in $Hooks) {
  if ($h -eq 'all') {
    $ids += Get-ChildItem (Join-Path $iniDir 'dru_*.ini') | ForEach-Object { $_.BaseName }
  } else {
    if ($h -notlike 'dru_*') { $h = "dru_$h" }
    $ids += $h
  }
}

if ($Apply) {
  $admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
  if (-not $admin) { Write-Error 'Run -Apply as an administrator'; exit 1 }
} else {
  'Dry run, nothing is changed (add -Apply)'
}
if (-not (Test-Path $uploadsD)) {
  "WARNING - $uploadsD is missing: install the agent (3.3-beta2 or later) first"
}

foreach ($id in ($ids | Select-Object -Unique)) {
  $ini = Join-Path $iniDir "$id.ini"
  if ($id -notmatch '^[a-z0-9_]{1,32}$' -or -not (Test-Path $ini)) {
    Write-Output "ERROR - no upload hook $id in $iniDir"
    exit 2
  }
  if (Test-Path (Join-Path $uploadsD "$id.ini")) {
    "$uploadsD\$id.ini is there already, left as it is"
  } elseif ($Apply) {
    "+ copy $ini to $uploadsD"
    Copy-Item $ini $uploadsD
  } else {
    "would: copy $ini to $uploadsD"
  }
}
"DRSM manifests for the monitor host: $(Split-Path -Parent $PSScriptRoot)\drsm\uploads\<id>"
