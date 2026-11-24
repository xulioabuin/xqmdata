<#
.SYNOPSIS
  Renames dynamic file subfiles from the legacy Linux naming (~0, ~1, ~N)
  to the OpenQM Windows naming (%0, %1, %N).

.DESCRIPTION
  XQMdata now names dynamic (hashed) file subfiles with '%' on Windows, as
  OpenQM for Windows does, because Windows cleanup tools may treat '~'
  files as temporary and delete them.

  The engine opens legacy '~' files transparently, but this script makes
  the naming uniform on disk. It scans directories recursively; any
  directory containing files named ~<digits> is treated as a dynamic
  file and those files are renamed to %<digits>.

  Run with the engine STOPPED (qm -stop) so no file is open.

.PARAMETER Root
  Directory tree(s) to scan. May be repeated. Default: C:\qmsys and
  C:\QMDATOS if they exist.

.PARAMETER WhatIf
  Show what would be renamed without doing it.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File win\rename-subfiles.ps1 -WhatIf
  powershell -ExecutionPolicy Bypass -File win\rename-subfiles.ps1 -Root C:\qmsys, C:\QMDATOS
#>
param(
  [string[]]$Root = @(),
  [switch]$WhatIf
)

$ErrorActionPreference = 'Stop'

if ($Root.Count -eq 0) {
  foreach ($r in @('C:\qmsys', 'C:\QMDATOS')) {
    if (Test-Path $r) { $Root += $r }
  }
}

$renamed = 0
$skipped = 0
$dirs = 0

foreach ($r in $Root) {
  if (-not (Test-Path $r)) { Write-Warning "No existe: $r"; continue }

  # Every directory that directly contains ~N files is a dynamic file
  $candidates = Get-ChildItem -Path $r -Recurse -File -Force |
    Where-Object { $_.Name -match '^~\d+$' }

  $groups = $candidates | Group-Object { $_.DirectoryName }

  foreach ($g in $groups) {
    $dirs++
    foreach ($f in $g.Group) {
      $newName = '%' + $f.Name.Substring(1)
      $target = Join-Path $f.DirectoryName $newName
      if (Test-Path $target) {
        Write-Warning "  Ya existe $target : no se renombra $($f.FullName)"
        $skipped++
        continue
      }
      if ($WhatIf) {
        Write-Host "  $($f.FullName)  ->  $newName"
      } else {
        Rename-Item $f.FullName $newName
      }
      $renamed++
    }
  }
}

# Report leftover transient '~' files (sort work files, safedir temps)
$leftovers = foreach ($r in $Root) {
  if (Test-Path $r) {
    Get-ChildItem -Path $r -Recurse -File -Force |
      Where-Object { $_.Name -like '~*' }
  }
}

Write-Host ""
Write-Host "Ficheros dinamicos procesados: $dirs"
Write-Host "Subficheros renombrados:       $renamed"
if ($skipped -gt 0) { Write-Host "Omitidos (destino existia):  $skipped" -ForegroundColor Yellow }
if ($leftovers) {
  Write-Host "Ficheros '~' residuales (temporales, revisar manualmente):" -ForegroundColor Yellow
  $leftovers | ForEach-Object { Write-Host "  $($_.FullName)" }
}
if ($WhatIf) { Write-Host "(modo WhatIf: no se ha renombrado nada)" -ForegroundColor Cyan }
