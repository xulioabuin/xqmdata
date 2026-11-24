<#
.SYNOPSIS
  Elimina la tarea programada de XQMdata y para el motor.

.DESCRIPTION
  Detiene el motor XQMdata (qm -stop, que para usuarios, qmlnxd y qmteld) y
  elimina la tarea programada "XQMdata" creada por install-services.ps1.

.PARAMETER QmsysDir
  Directorio de instalación de XQMdata (para localizar qm.exe al pararlo).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File remove-services.ps1
#>
param(
  [string]$QmsysDir = 'C:\qmsys'
)

$ErrorActionPreference = 'Continue'

$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$pri = New-Object Security.Principal.WindowsPrincipal($id)
if (-not $pri.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  throw "This script neeed run as administrator."
}

# Parar el motor (qm -stop detiene usuarios, qmlnxd y qmteld)
$qm = Join-Path $QmsysDir 'bin\qm.exe'
if (Test-Path $qm) {
  & $qm -stop | Out-Null
}

# Por si quedara algún qmteld lanzado a mano
Get-Process qmteld -ErrorAction SilentlyContinue | Stop-Process -Force

Unregister-ScheduledTask -TaskName 'XQMdata' -Confirm:$false `
  -ErrorAction SilentlyContinue

Write-Host 'Task XQMdata deleted.'
