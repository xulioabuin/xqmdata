# XQMdata

**XQMdata** is a **MultiValue** database system (PICK family) for **Windows**.
It is built from [ScarletDME](https://github.com/geneb/ScarletDME) (the GPL fork of Ladybridge Systems' OpenQM 2.6) and ported to Windows with MinGW-w64.

This program is free software and is supplied with ABSOLUTELY NO WARRANTY.
The installed product does not require MSYS2 to run; MSYS2 is only needed to build from source.

---

## Installation and automatic startup

### Install from the repository

Run PowerShell as administrator and execute the installer:

```powershell
git clone https://github.com/xulioabuin/xqmdata.git
cd xqmdata
powershell -ExecutionPolicy Bypass -File win\install.ps1
```

The installer will:

1. Copy the `qmsys/` template to the target directory (default `C:\qmsys`).
2. Copy the compiled binaries (`qm.exe`, `qmlnxd.exe`, `qmteld.exe`, `xqmclilib.dll`, utilities).
3. Write `C:\ProgramData\XQM\xqm.conf`.
4. Register the QMSYS account in the ACCOUNTS file.
5. Optionally register the Windows scheduled task for automatic startup.

### Uninstall

```powershell
powershell -ExecutionPolicy Bypass -File services\remove-services.ps1
```

This stops the engine and removes the scheduled task.

### Configuration

Edit `C:\ProgramData\XQM\xqm.conf`:

```ini
[qm]
QMSYS=C:\qmsys
GRPSIZE=2
NUMUSERS=10
SORTMEM=4096
PORT=4242
CLIPORT=4243
TELNET=1
```

### Start or stop the engine

```powershell
C:\qmsys\bin\qm.exe -start   # also starts qmlnxd and, with TELNET=1, qmteld
C:\qmsys\bin\qm.exe -stop
```

---

## QMClient API access

The native client library is **`xqmclilib.dll`** (ANSI `char*` + Unicode `wchar_t` API).

- **ANSI** functions (`char*`): `QMConnect`, `QMCall`, `QMExecute`, `QMRead`...
- **Unicode** functions (`wchar_t`): `QMConnectW`, `QMCallxW`, `QMExecuteW`, `QMReadW`...

Connections:

- **TCP** on port `4243` (Windows credential login via `LogonUserA` + `$LOGINS` registration).
- **QMLocal** pipe-based local clients.

---

## Features

- Full `qm.exe` engine: TCL, QMBasic, hashed and directory files, I-types.
- **Telnet** access via `qmteld.exe` (port `4242`) — the Windows equivalent of Linux `xinetd`.
- **QMClient** access (port `4243`) plus **QMLocal** (pipe-based local clients).
- Native client library **`xqmclilib.dll`** (ANSI + Unicode API).
- Native Windows paths (`C:\...`) with correct CR/LF handling for directory files.
- Automatic startup via Windows scheduled tasks (`services/`).

Status: **functional, in development**. XQMdata has been used to migrate real-world accounts from OpenQM 4.0.9 (Windows) and ScarletDME (Linux), including web frontends (Vue + Flask + `xqmclilib.dll`) calling QMBasic routines via QMClient.

---

## Build from source

### Requirements

| Component   | Details |
|-------------|---------|
| **MSYS2**   | Installed at `C:\msys64` (<https://www.msys2.org>) |
| **Toolchain** | `pacman -S mingw-w64-ucrt-x86_64-gcc make` (GCC ≥ 15 at `/mingw64/bin`) |
| **PowerShell** | 5.1+ (for the service scripts) |

No MinGW runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`) are needed at runtime: all binaries are statically linked.

### Build steps

```bash
cd /c/<repo-path>/XQMdata/win
./compile-all.sh      # compiles gplsrc/*.c + win/winipc.c   → win/obj/*.o
./link-all.sh         # links qm.exe, qmlnxd.exe, qmteld.exe  → win/obj/
./build-clientlib.sh  # builds xqmclilib.dll                  → win/obj/
./build-utilities.sh  # builds qmfix, qmconv, qmidx, qmtic    → win/obj/
```

To rebuild just a few modules:

```bash
./rebuild-fix.sh module1 module2
```

### Build output in `win/obj/`

| File            | Description |
|-----------------|-------------|
| `qm.exe`        | Full engine (TCL, sessions, `-start`/`-stop`) |
| `qmlnxd.exe`    | Housekeeping daemon (started by `qm -start`) |
| `qmteld.exe`    | Telnet (4242) + QMClient (4243) listener |
| `xqmclilib.dll` | Native QMClient library (ANSI + Unicode API) |
| `qmfix.exe`     | Hash file check/repair tool |
| `qmconv.exe`    | Byte-order file converter |
| `qmidx.exe`     | Index relocation tool |
| `qmtic.exe`     | Terminfo compiler |

`win/obj/` is gitignored build output.

---

## Repository layout

```text
XQMdata/
├── gplsrc/          C engine sources (includes the Windows port fixes)
├── gpl.src          Module list to compile
├── bin/             Master pcode files (needed at runtime)
├── qmsys/           QMSYS template (hashed files, NEWVOC, terminfo, bin/)
├── pcode_files/     Sources of the pcode routines
├── services/        Windows scheduled tasks (install/remove/README)
├── win/             Windows port
│   ├── compile-all.sh / link-all.sh / rebuild-fix.sh
│   ├── build-clientlib.sh / build-utilities.sh
│   ├── winipc.c     POSIX→Win32 layer (shm, semaphores, console, sockets)
│   ├── qmteld.c     Telnet + QMClient listener (xinetd equivalent)
│   ├── xqmclilib_w.c Unicode wrappers for the client library
│   ├── include/     Compatibility headers (termios, poll, pwd...)
│   └── obj/         Build output (gitignored)
├── terminfo.src     Terminfo source
└── xqm.conf         Configuration template
```

---

## Known limitations

- **UNC** paths (`\\server\share`) are not supported in validation/creation.
- OpenQM 4.x ships verbs/objects that the GPL 2.6 base does not include (`BSCAN`, `$LISTTERM`, `$PTERM`...): this is a product difference, not a bug.
- `qmteld` is managed as a scheduled task, not as a native SCM service.
- Network login (telnet/QMClient) validates against **Windows accounts** (`LogonUserA`) + registration in `$LOGINS`, like OpenQM's QMSvc.

---

## Credits and license

- **ScarletDME** — <https://github.com/geneb/ScarletDME> (GPLv2+).
- **OpenQMhelp** — <https://scarl.cdmiweb.com/>

XQMdata inherits the **GPL v2 or later** license. See `qmsys/licence`.
The original copyright notice is kept at startup, as required by the base project's license.
