<#
.SYNOPSIS
  Registers XQMdata as a Windows scheduled task that runs at startup.

.DESCRIPTION
  Creates a single scheduled task, "XQMdata", which executes 'qm.exe -start' when
  Windows starts. The engine's own startup process handles everything:
    - Shared memory segment and semaphores
    - qmlnxd.exe (maintenance)
    - qmteld.exe (Telnet listener, if TELNET=1 in xqm.conf)

  NOTE: qm.exe does not implement the Windows SCM (Service Control Manager)
  protocol; therefore, a scheduled task is used instead of a traditional service.

.PARAMETER QmsysDir
  The XQMdata installation directory (contains the bin\ folder and xqm.conf).

.PARAMETER NoStart
  If specified, the task is registered but not started immediately.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File install-services.ps1
  powershell -ExecutionPolicy Bypass -File install-services.ps1 -QmsysDir D:\XQMdata\qmsys
#>
param(
  [string]$QmsysDir = 'D:\qmsys',
  [switch]$NoStart
)

$ErrorActionPreference = 'Stop'

# --- Comprobaciones -------------------------------------------------------

$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$pri = New-Object Security.Principal.WindowsPrincipal($id)
if (-not $pri.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  throw "Este script necesita ejecutarse como administrador."
}

$qm = Join-Path $QmsysDir 'bin\qm.exe'
if (-not (Test-Path $qm)) { throw "NoT found $qm" }

# --- Tarea única: arranque del motor al iniciar Windows -------------------

$action = New-ScheduledTaskAction -Execute $qm -Argument '-start' `
  -WorkingDirectory $QmsysDir
$trigger = New-ScheduledTaskTrigger -AtStartup
$principal = New-ScheduledTaskPrincipal -UserId 'SYSTEM' `
  -LogonType ServiceAccount -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries `
  -DontStopIfGoingOnBatteries

Register-ScheduledTask -TaskName 'XQMdata' -Action $action -Trigger $trigger `
  -Principal $principal -Settings $settings `
  -Description "Start engine XQMdata ($qm -start) when start Windows." `
  -Force | Out-Null

# --- Arranque inmediato ----------------------------------------------------

if (-not $NoStart) {
  Start-ScheduledTask -TaskName 'XQMdata'
  Start-Sleep -Seconds 2
}

Write-Host ''
if ($NoStart) {
  Write-Host 'Task XQMdata registered (No Start).'
} else {
  Write-Host 'Task XQMdata registrada and Started:'
  Get-ScheduledTask -TaskName 'XQMdata' |
    Select-Object TaskName, State | Format-Table -AutoSize
}
Write-Host 'Gestión: services\remove-services.ps1 to uninstall.'
