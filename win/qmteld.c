/* QMTELD.C
 *
 * Listener daemon for ScarletDME on Windows (telnet + QMClient).
 *
 * Equivalent of the Linux xinetd setup: listens on the QM telnet port
 * (default 4242, spawning "qm.exe -N<socket>") and on the QMClient port
 * (default 4243, spawning "qm.exe -Q -N<socket>"), passing each accepted
 * socket to a new engine process.
 *
 * Usage:
 *    qmteld [port]
 *
 * Ports come from xqm.conf (PORT / CLIPORT; a numeric argument overrides
 * PORT). Set CLIPORT=0 in xqm.conf to disable the QMClient listener.
 *
 * The QM system must have been started (qm -start) before clients connect.
 */

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QMTELD_DEFAULT_PORT 4242
#define QMTELD_DEFAULT_CLIPORT 4243

/* Locate the configuration file, mirroring inipath.c */
static void conf_path(char *out, size_t out_len) {
  const char *p = getenv("XQM_CONFIG");

  if (p != NULL && *p != '\0') {
    snprintf(out, out_len, "%s", p);
    return;
  }

  p = getenv("QMSYS");
  if (p != NULL && *p != '\0') {
    snprintf(out, out_len, "%s\\xqm.conf", p);
    return;
  }

  snprintf(out, out_len, "C:\\ProgramData\\XQM\\xqm.conf");
}

/* Read PORT and CLIPORT from the [qm] section of xqm.conf */
static void read_conf_ports(int *port, int *cliport) {
  char path[MAX_PATH];
  char line[256];
  FILE *f;
  int in_qm = 0;
  int v;

  conf_path(path, sizeof(path));
  f = fopen(path, "r");
  if (f == NULL) {
    return; /* Keep defaults */
  }

  while (fgets(line, sizeof(line), f) != NULL) {
    if (line[0] == '[') {
      in_qm = (strnicmp(line, "[qm]", 4) == 0);
      continue;
    }
    if (!in_qm) {
      continue;
    }
    if (sscanf(line, "PORT=%d", &v) == 1) {
      *port = v;
    } else if (sscanf(line, "CLIPORT=%d", &v) == 1) {
      *cliport = v;
    }
  }
  fclose(f);
}

static void log_line(const char *fmt, ...) {
  char msg[1024];
  char stamp[32];
  time_t now = time(NULL);
  va_list ap;

  strftime(stamp, sizeof(stamp), "%d %b %Y %H:%M:%S", localtime(&now));

  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  fprintf(stderr, "%s %s\n", stamp, msg);
  fflush(stderr);
}

/* Create, bind and listen on a TCP port. The listen socket is marked
   non-inheritable so children do not keep it. Returns INVALID_SOCKET on
   error. */
static SOCKET create_listener(int port) {
  SOCKET ls;
  struct sockaddr_in sa;
  int one = 1;

  ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ls == INVALID_SOCKET) {
    return INVALID_SOCKET;
  }

  SetHandleInformation((HANDLE)ls, HANDLE_FLAG_INHERIT, 0);
  setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons((unsigned short)port);

  if (bind(ls, (struct sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR ||
      listen(ls, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(ls);
    return INVALID_SOCKET;
  }

  return ls;
}

/* Accept one connection on 'ls' and spawn the engine for it.
   'args_fmt' receives the socket value (e.g. "-N%u" or "-Q -N%u"). */
static void spawn_connection(SOCKET ls, const char *qmexe,
                             const char *args_fmt, const char *tag) {
  SOCKET s = accept(ls, NULL, NULL);
  char cmd[MAX_PATH + 64];
  char args[64];
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  if (s == INVALID_SOCKET) {
    return;
  }

  /* Ensure the accepted socket is inheritable (the listen socket's
     non-inheritable flag could otherwise propagate to it) */
  SetHandleInformation((HANDLE)s, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

  snprintf(args, sizeof(args), args_fmt, (unsigned int)s);
  snprintf(cmd, sizeof(cmd), "\"%s\" %s", qmexe, args);

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  /* bInheritHandles = TRUE: the child inherits the accepted socket with
     the same numeric value (the listen sockets are non-inheritable). */
  if (CreateProcessA(NULL, cmd, NULL, NULL, TRUE, DETACHED_PROCESS, NULL,
                     NULL, &si, &pi)) {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    log_line("qmteld: %s connection on socket %u -> qm.exe %s", tag,
             (unsigned int)s, args);
  } else {
    log_line("qmteld: CreateProcess failed (%lu)", GetLastError());
  }

  /* The child holds its own inherited reference to the socket */
  closesocket(s);
}

int main(int argc, char *argv[]) {
  int port = QMTELD_DEFAULT_PORT;
  int cliport = QMTELD_DEFAULT_CLIPORT;
  char qmexe[MAX_PATH];
  char dir[MAX_PATH];
  char *p;
  int i;

  WSADATA wsa;
  SOCKET ls_telnet = INVALID_SOCKET;
  SOCKET ls_client = INVALID_SOCKET;

  /* Ports from xqm.conf first; command line may override PORT */
  read_conf_ports(&port, &cliport);

  /* Parse arguments */
  for (i = 1; i < argc; i++) {
    if (!stricmp(argv[i], "-qmsys") && (i + 1 < argc)) {
      i++; /* Reserved for future use: qm.exe is taken from our own dir */
    } else if (isdigit((unsigned char)argv[i][0])) {
      port = atoi(argv[i]);
    }
  }

  /* Locate qm.exe next to ourselves */
  if (GetModuleFileNameA(NULL, dir, sizeof(dir)) == 0) {
    fprintf(stderr, "qmteld: cannot determine own path\n");
    return 1;
  }
  p = strrchr(dir, '\\');
  if (p != NULL) {
    *p = '\0';
  }
  snprintf(qmexe, sizeof(qmexe), "%s\\qm.exe", dir);

  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    fprintf(stderr, "qmteld: WSAStartup failed\n");
    return 1;
  }

  /* Telnet listener (terminal sessions) */
  ls_telnet = create_listener(port);
  if (ls_telnet == INVALID_SOCKET) {
    log_line("qmteld: cannot listen on telnet port %d (%d)", port,
             WSAGetLastError());
  }

  /* QMClient listener (API sessions, -Q mode) */
  if (cliport > 0) {
    ls_client = create_listener(cliport);
    if (ls_client == INVALID_SOCKET) {
      log_line("qmteld: cannot listen on QMClient port %d (%d)", cliport,
               WSAGetLastError());
    }
  }

  if (ls_telnet == INVALID_SOCKET && ls_client == INVALID_SOCKET) {
    fprintf(stderr, "qmteld: no listeners available, exiting\n");
    return 1;
  }

  if (ls_telnet != INVALID_SOCKET) {
    log_line("qmteld: telnet on port %d -> qm.exe -N<socket>", port);
  }
  if (ls_client != INVALID_SOCKET) {
    log_line("qmteld: QMClient on port %d -> qm.exe -Q -N<socket>", cliport);
  }
  log_line("qmteld: spawning %s", qmexe);

  for (;;) {
    WSAPOLLFD fds[2];
    int nfds = 0;
    int r;

    if (ls_telnet != INVALID_SOCKET) {
      fds[nfds].fd = ls_telnet;
      fds[nfds].events = POLLIN;
      fds[nfds].revents = 0;
      nfds++;
    }
    if (ls_client != INVALID_SOCKET) {
      fds[nfds].fd = ls_client;
      fds[nfds].events = POLLIN;
      fds[nfds].revents = 0;
      nfds++;
    }

    r = WSAPoll(fds, nfds, 1000);
    if (r == SOCKET_ERROR || r <= 0) {
      continue;
    }

    nfds = 0;
    if (ls_telnet != INVALID_SOCKET) {
      if (fds[nfds].revents & POLLIN) {
        spawn_connection(ls_telnet, qmexe, "-N%u", "telnet");
      }
      nfds++;
    }
    if (ls_client != INVALID_SOCKET) {
      if (fds[nfds].revents & POLLIN) {
        spawn_connection(ls_client, qmexe, "-Q -N%u", "QMClient");
      }
      nfds++;
    }
  }

  /* Never reached */
  return 0;
}
