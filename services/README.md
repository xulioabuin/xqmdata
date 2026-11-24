# XQMdata — Automatic startup on Windows (services)

This folder replaces the old `etc/xinetd.d` and `usr/lib/systemd`
(Linux-only configuration, removed from the tree).

## A single task: `XQMdata`

With `TELNET=1` in `xqm.conf`, the `qm -start` command already brings up
**everything**: the shared segment, `qmlnxd.exe` (housekeeping) and
`qmteld.exe` (telnet listener on the `PORT` port, 4242 by default). So a
single scheduled task running `qm -start` at Windows startup is enough.

`qm -stop` takes care of stopping everything (users, qmlnxd and qmteld).

| Task  | Linux equivalent                  | Action                                    |
|-------|-----------------------------------|-------------------------------------------|
| `XQMdata` | `scarletdme.service` + `xinetd qmserver` | `qm.exe -start` at Windows startup (one-shot; leaves the system running) |

The QMClient listener (`xinetd qmclient` on Linux, port 4243) is also active:
`xqm.conf` sets `CLIPORT=4243` and `qmteld` listens on it.

## Why a scheduled task and not a classic service

`qm.exe` and `qmteld.exe` do not implement the SCM (Service Control Manager)
protocol, so they cannot be registered directly with `sc.exe` (they would fail
with error 1053). A **scheduled task** does the same job without external
wrappers (NSSM and the like), and runs as **SYSTEM**, with no interactive session.

## Usage

```powershell
# Install and start (as administrator)
powershell -ExecutionPolicy Bypass -File install-services.ps1

# Install in another directory / don't start now
powershell -ExecutionPolicy Bypass -File install-services.ps1 -QmsysDir D:\XQMdata\qmsys -NoStart

# Uninstall (stops the engine and removes the task)
powershell -ExecutionPolicy Bypass -File remove-services.ps1
```

## Notes

- The engine reads its configuration from `C:\ProgramData\XQM\xqm.conf` — no
  environment variables are needed.
- The XQMdata installer will use these scripts passing `-QmsysDir`
  with the directory chosen by the user.
