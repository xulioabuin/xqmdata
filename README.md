### XQMdata

**XQMdata** is a **MultiValue** database system (PICK family) for **Windows**.
Built from [ScarletDME](https://github.com/geneb/ScarletDME) (the GPL fork of Ladybridge Systems' OpenQM 2.6), ported to Windows with MinGW-w64.

This program is free software and is supplied with ABSOLUTELY NO WARRANTY.
The installed product does not require MSYS2 to run, only is needed to build

### INSTALATION AND AUTOMATIC SARTUP WITH WINDOWS

# Install and Register the scheduled task (as administrator).
download this Repository
cd \<Repository>
powershell -ExecutionPolicy Bypass -File win\install.ps1
# This is all!
 
# Uninstall (stops the engine and removes the task)
powershell -ExecutionPolicy Bypass -File services\remove-services.ps1

# Configuration
Change `C:\ProgramData\XQM\xqm.conf`:
[qm]
QMSYS=D:\qmsys
GRPSIZE=2
NUMUSERS=10
SORTMEM=4096
PORT=4242
CLIPORT=4243
TELNET=1

# Start or Stop the engine (Start also launches qmlnxd and, with TELNET=1, qmteld)
C:\qmsys\bin\qm.exe -start
C:\qmsys\bin\qm.exe -stop


## QMClient (API access)

Native client library `xqmclilib.dll` 'qmclient.dll' compatible

**ANSI** functions (`char*`): `QMConnect`, `QMCall`, `QMExecute`, `QMRead`...
**Unicode** functions (`wchar_t`): `QMConnectW`, `QMCallxW`, `QMExecuteW`,`QMReadW`... (what modern clients use).
Connections: **TCP** (port 4243, Windows credential login via `LogonUserA` + `$LOGINS` registration) and **QMLocal** (pipe-based, current user).

# Current features:
- Full `qm.exe` engine (TCL, QMBasic, hashed and directory files, I-types).
- **Telnet** access (`qmteld.exe`, port 4242), the Windows equivalent of xinetd.
- **QMClient** access (port 4243) plus **QMLocal** (pipe-based local clients).
- Native client library **`xqmclilib.dll`** (ANSI `char*` + Unicode `wchar_t` API) for XQMdata.
- Native Windows paths (`C:\...`), directory files with correct CR/LF handling.
- Automatic startup via scheduled tasks (`services/`).

Status: **functional, in development**. XQMdata successfully migrates accounts from
OpenQM 4.0.9 (Windows) and ScarletDME (Linux) in real-world testing, including web frontends
(Vue + Flask + xqmclilib.dll) calling QMBasic routines via QMClient.


### DEVELOPERS

# build requirements if you want modify source

**MSYS2** 	| Installed at `C:\msys64` (<https://www.msys2.org>)
**Toolchain** | `pacman -S mingw-w64-ucrt-x86_64-gcc make` (GCC ≥ 15 at `/mingw64/bin`)
**PowerShell**| 5.1+ (for the services scripts)

No MinGW runtime DLLs (libgcc_s_seh-1, libstdc++-6, libwinpthread-1) are needed:
all binaries are statically linked (`-static-libgcc -static-libstdc++ -static winpthread`).

cd /c/<repo-path>/XQMdata/win
./compile-all.sh      # compiles gplsrc/*.c + win/winipc.c   → win/obj/*.o
./link-all.sh         # links qm.exe, qmlnxd.exe, qmteld.exe  → win/obj/
./build-clientlib.sh  # builds the client library xqmclilib.dll → win/obj/
./build-utilities.sh  # builds qmfix, qmconv, qmidx, qmtic    → win/obj/

Output in `win/obj/`:

`qm.exe` 		| Full engine (TCL, sessions, `-start`/`-stop`)
`qmlnxd.exe`	| Housekeeping daemon (started by `qm -start`)
`qmteld.exe`	| Telnet (4242) + QMClient (4243) listener
`xqmclilib.dll`	| Native QMClient library (ANSI + Unicode API)
`qmfix.exe`		| Hash file check/repair tool
`qmconv.exe`	| Byte-order file convertor
`qmidx.exe`		| Index relocation tool
`qmtic.exe`		| Terminfo compiler

`win/obj/` is gitignored: it is build output.
To rebuild just a few modules: `./rebuild-fix.sh module1 module2`.


 ## Repository layout

XQMdata/
├── gplsrc/          C engine sources (includes the port fixes)
├── gpl.src          Module list to compile
├── bin/             master pcode (needed at runtime)
├── qmsys/           QMSYS template (hashed files, NEWVOC, terminfo, bin/)
├── pcode_files/     Sources of the pcode routines
├── services/        Windows scheduled tasks (install/remove/README)
├── win/             Windows port:
│   ├── compile-all.sh / link-all.sh / rebuild-fix.sh / build-clientlib.sh / build-utilities.sh
│   ├── winipc.c     POSIX→Win32 layer (shm, semaphores, console, sockets)
│   ├── qmteld.c     Telnet + QMClient listener (xinetd equivalent)
│   ├── xqmclilib_w.c Unicode wrappers for the client library
│   ├── include/     Compatibility headers (termios, poll, pwd...)
│   └── obj/         Build output (gitignored)
├── xqm.conf         Configuration template



## Known limitations

- **UNC** paths (`\\server\share`) not supported in validation/creation.
- OpenQM 4.x ships verbs/objects that the GPL 2.6 base does not include
  (`BSCAN`, `$LISTTERM`, `$PTERM`...): this is a product difference, not a bug.
- `qmteld` is managed as a scheduled task (not a native SCM service).
- Network login (telnet/QMClient) validates against **Windows accounts**
  (`LogonUserA`) + registration in `$LOGINS`, like OpenQM's QMSvc.

## Credits and license

- **ScarletDME** — <https://github.com/geneb/ScarletDME> (GPLv2+).
- **OpenQMhelp** — Openqm help (https://scarl.cdmiweb.com/)

XQMdata inherits the **GPL v2 or later** license. See `qmsys/licence`.
The original copyright notice is kept at startup (`CONFIG GPL` for details),
as required by the base project's license.
