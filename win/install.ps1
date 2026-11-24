<#
.SYNOPSIS
  XQMdata installer - installs the XQMdata MultiValue database engine on Windows.

.DESCRIPTION
  Interactive installer (asks for the target directory):
    1. Copies the QMSYS template to the target directory.
    2. Copies the binaries (qm.exe, qmlnxd.exe, qmteld.exe, xqmclilib.dll
       and the qmfix/qmconv/qmidx/qmtic utilities). The binaries are
       statically linked: no MinGW runtime DLLs are needed.
    3. Writes the configuration file C:\ProgramData\XQM\xqm.conf.
    4. Registers the QMSYS account in the ACCOUNTS file with the real path.
    5. Optionally registers the Windows scheduled task (auto-start).
    6. Runs a smoke test (qm -start / COUNT VOC / qm -stop).

  Must be run from the XQMdata source tree (it locates the template and the
  compiled binaries relative to the script).

.PARAMETER QmsysDir
  Target directory. If omitted, the installer asks interactively
  (default C:\qmsys).

.PARAMETER NoService
  Skip the scheduled task registration.

.PARAMETER Quiet
  Do not prompt; use defaults (or the given parameters).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File win\install.ps1
  powershell -ExecutionPolicy Bypass -File win\install.ps1 -QmsysDir D:\XQMdata\qmsys
#>
param(
  [string]$QmsysDir = '',
  [switch]$NoService,
  [switch]$Quiet
)

$ErrorActionPreference = 'Stop'
$ScriptRoot = $PSScriptRoot

function Write-Step($msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "  [OK] $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "  [AVISO] $msg" -ForegroundColor Yellow }
function Write-Fail($msg) { Write-Host "  [FALLO] $msg" -ForegroundColor Red }

# ---------------------------------------------------------------- 1. Target
Write-Step "XQMdata - Instalador"

if ([string]::IsNullOrWhiteSpace($QmsysDir)) {
  if ($Quiet) {
    $QmsysDir = 'C:\qmsys'
  } else {
    $answer = Read-Host "Directorio de instalacion [C:\qmsys]"
    $QmsysDir = if ([string]::IsNullOrWhiteSpace($answer)) { 'C:\qmsys' } else { $answer }
  }
}
$QmsysDir = $QmsysDir.TrimEnd('\', '/')
Write-Ok "Directorio: $QmsysDir"

# Source locations
$templateDir = Join-Path $ScriptRoot '..\qmsys'
$objDir      = Join-Path $ScriptRoot 'obj'
foreach ($p in @($templateDir, $objDir)) {
  if (-not (Test-Path $p)) { Write-Fail "No se encuentra $p (ejecuta desde win\ del repo, tras compilar)"; exit 1 }
}

$exes = @('qm.exe', 'qmlnxd.exe', 'qmteld.exe', 'xqmclilib.dll', 'qmfix.exe', 'qmconv.exe', 'qmidx.exe', 'qmtic.exe')
foreach ($e in $exes) {
  if (-not (Test-Path (Join-Path $objDir $e))) { Write-Fail "Falta $e en $objDir (compila antes: compile-all.sh, link-all.sh, build-clientlib.sh, build-utilities.sh)"; exit 1 }
}

# ---------------------------------------------------- 2. Copy QMSYS template
Write-Step "Copiando plantilla QMSYS"
if (Test-Path $QmsysDir) {
  Write-Warn "El directorio ya existe: se actualizan los archivos"
} else {
  New-Item -ItemType Directory -Path $QmsysDir -Force | Out-Null
}
# Copiar el CONTENIDO de la plantilla (Copy-Item del directorio crearia
# un subdirectorio qmsys anidado si el destino ya existe).
Copy-Item "$templateDir\*" $QmsysDir -Recurse -Force
Write-Ok "Plantilla copiada a $QmsysDir"

# ------------------------------------------------------------ 3. Binaries
Write-Step "Copiando binarios"
$binDir = Join-Path $QmsysDir 'bin'
if (-not (Test-Path $binDir)) { New-Item -ItemType Directory -Path $binDir -Force | Out-Null }
foreach ($e in $exes) {
  Copy-Item (Join-Path $objDir $e) $binDir -Force
  Write-Ok "  $e"
}

# ------------------------------------------------------------ 4. xqm.conf
Write-Step "Escribiendo configuracion"
$confDir = 'C:\ProgramData\XQM'
if (-not (Test-Path $confDir)) { New-Item -ItemType Directory -Path $confDir -Force | Out-Null }
$conf = @"
[qm]
QMSYS=$QmsysDir
GRPSIZE=2
NUMUSERS=10
SORTMEM=4096
PORT=4242
CLIPORT=4243
TELNET=1
"@
Set-Content -Path (Join-Path $confDir 'xqm.conf') -Value $conf -NoNewline
Write-Ok "$confDir\xqm.conf"

# --------------------------------------- 5. Register QMSYS in ACCOUNTS
Write-Step "Registrando la cuenta QMSYS en ACCOUNTS"

$qmExe = Join-Path $binDir 'qm.exe'
$updateProg = @"
OPENPATH '$QmsysDir\ACCOUNTS' TO ACC.F ELSE STOP 'NO SE ABRE ACCOUNTS'
READ ACC.REC FROM ACC.F, 'QMSYS' ELSE ACC.REC = ''
ACC.REC<1> = '$QmsysDir'
WRITE ACC.REC TO ACC.F, 'QMSYS'
PRINT 'QMSYS registrado en ' : '$QmsysDir'
END
"@
$bpFile = Join-Path $QmsysDir 'BP\XQMINSTALL'
Set-Content -Path $bpFile -Value $updateProg -NoNewline

# Las operaciones QM (BASIC/RUN) se hacen con el directorio de trabajo en la
# nueva instalacion, para que la cuenta activa sea la nueva (BP = su BP).
$prevLocation = Get-Location
Set-Location $QmsysDir
& $qmExe -start | Out-Null
$compile = "BASIC BP XQMINSTALL`nRUN BP XQMINSTALL`nQUIT`n"
$compile | & $qmExe | Out-Null
& $qmExe -stop | Out-Null
Set-Location $prevLocation
Remove-Item $bpFile -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $QmsysDir 'BP.OUT\XQMINSTALL') -Force -ErrorAction SilentlyContinue
Write-Ok "ACCOUNTS actualizado"

# ---------------------------------------------------- 6. Scheduled task
if (-not $NoService) {
  Write-Step "Tarea programada de arranque automatico"
  $installSvc = Join-Path $ScriptRoot '..\services\install-services.ps1'
  if (Test-Path $installSvc) {
    try {
      & powershell -ExecutionPolicy Bypass -File $installSvc -QmsysDir $QmsysDir -NoStart
      Write-Ok "Tarea 'XQMdata' registrada (no arrancada; arrancala o reinicia)"
    } catch {
      Write-Warn "No se pudo registrar la tarea (¿ejecutado como administrador?): $($_.Exception.Message)"
    }
  }
}

# ------------------------------------------------------------ 7. Smoke test
Write-Step "Prueba de arranque"
$prevLocation = Get-Location
Set-Location $QmsysDir
& $qmExe -start | Out-Null
$result = "COUNT VOC`nQUIT`n" | & $qmExe 2>&1 | Select-String -Pattern 'record\(s\) counted'
& $qmExe -stop | Out-Null
Set-Location $prevLocation
if ($result) {
  Write-Ok "Motor arranca y lee datos: $($result.Line.Trim())"
} else {
  Write-Warn "El motor arranco pero COUNT VOC no respondio como se esperaba"
}

Write-Step "Instalacion completada"
Write-Host @"

  XQMdata instalado en:  $QmsysDir
  Configuracion:         C:\ProgramData\XQM\xqm.conf

  Arrancar:          $qmExe -start
  Sesion TCL:        cd $QmsysDir ; $qmExe
  Parar:             $qmExe -stop

"@ -ForegroundColor Cyan
