/* winipc.c
 *
 * Win32 implementations of the small set of POSIX facilities that the
 * ScarletDME engine needs and that MinGW-w64 does not provide.
 *
 * Built into every Windows engine binary (qm.exe, qmlnxd.exe, ...).
 */

#include <windows.h>
#include <winsock2.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "qmwin32.h"
#include <termios.h>
#include "crypt.h"
#include "pwd.h"
#include "dlfcn.h"

#define QM_MAX_SHM 16
#define QM_MAX_SEM 16
#define QM_MAX_SEMAPHORES 256

static SECURITY_ATTRIBUTES *win_sa(void) {
  static SECURITY_DESCRIPTOR sd;
  static SECURITY_ATTRIBUTES sa;
  static int initialised = 0;

  if (!initialised) {
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;
    initialised = 1;
  }
  return &sa;
}

static void win_base_dir(char *out, size_t out_len) {
  DWORD n;
  size_t len;

  n = GetEnvironmentVariableA("ProgramData", out, (DWORD)out_len);
  if (n == 0 || n >= out_len) {
    n = GetTempPathA((DWORD)out_len, out);
    if (n == 0 || n >= out_len) {
      out[0] = '\0';
    }
  }

  len = strlen(out);
  while (len > 0 && (out[len - 1] == '\\' || out[len - 1] == '/')) {
    out[--len] = '\0';
  }
  if (len + 12 < out_len) {
    strcat(out, "\\XQM");
  }
  CreateDirectoryA(out, win_sa());
}

/* ---------------------------------------------------------------------- */
/* Process / user identity                                                */

unsigned int getuid(void) { return 0; }
unsigned int geteuid(void) { return 0; }
unsigned int getgid(void) { return 0; }
unsigned int getegid(void) { return 0; }

int getpid(void) { return (int)GetCurrentProcessId(); }

int qm_gethostname(char *name, int len) {
  DWORD size;

  if (name == NULL || len <= 0) {
    errno = EINVAL;
    return -1;
  }
  size = (DWORD)len;
  if (!GetComputerNameA(name, &size)) {
    strncpy(name, "localhost", (size_t)len);
    name[len - 1] = '\0';
  }
  return 0;
}

int fsync(int fd) { return _commit(fd); }

char *ttyname(int fd) {
  (void)fd;
  return "CON";
}

int fcntl(int fd, int cmd, ...) {
  va_list ap;
  int arg = 0;

  if (cmd == F_SETFL || cmd == F_SETFD || cmd == F_SETOWN) {
    va_start(ap, cmd);
    arg = va_arg(ap, int);
    va_end(ap);
  }

  if (cmd == F_SETFL) {
    u_long mode = (arg & O_NONBLOCK) ? 1 : 0;
    if (ioctlsocket((SOCKET)(intptr_t)fd, FIONBIO, &mode) == 0) {
      return 0;
    }
    errno = EINVAL;
    return -1;
  }

  return 0;
}

int qm_spawn_detached(const char *exe, const char *args, int *pid_out) {
  char cmd[4096];
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  if (exe == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (args != NULL && args[0] != '\0') {
    snprintf(cmd, sizeof(cmd), "\"%s\" %s", exe, args);
  } else {
    snprintf(cmd, sizeof(cmd), "\"%s\"", exe);
  }

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, DETACHED_PROCESS, NULL,
                      NULL, &si, &pi)) {
    errno = EIO;
    return -1;
  }

  if (pid_out != NULL) {
    *pid_out = (int)pi.dwProcessId;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 0;
}

int kill(pid_t pid, int sig) {
  HANDLE h;
  DWORD code;

  if (pid <= 0) {
    errno = ESRCH;
    return -1;
  }

  if (sig == 0) {
    h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (h == NULL) {
      errno = ESRCH;
      return -1;
    }
    if (!GetExitCodeProcess(h, &code) || code != STILL_ACTIVE) {
      CloseHandle(h);
      errno = ESRCH;
      return -1;
    }
    CloseHandle(h);
    return 0;
  }

  if (sig == SIGTERM || sig == SIGKILL) {
    h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
    if (h == NULL) {
      errno = EPERM;
      return -1;
    }
    if (!TerminateProcess(h, 1)) {
      CloseHandle(h);
      errno = EPERM;
      return -1;
    }
    CloseHandle(h);
    return 0;
  }

  return 0;
}

pid_t waitpid(pid_t pid, int *status, int options) {
  (void)pid;
  (void)options;
  if (status != NULL) {
    *status = 0;
  }
  errno = ECHILD;
  return -1;
}

int uname(struct utsname *buf) {
  DWORD v;
  DWORD size;

  if (buf == NULL) {
    errno = EINVAL;
    return -1;
  }
  memset(buf, 0, sizeof(*buf));

  strcpy(buf->sysname, "Windows");

  size = (DWORD)sizeof(buf->nodename);
  if (!GetComputerNameA(buf->nodename, &size)) {
    strcpy(buf->nodename, "localhost");
  }

  v = GetVersion();
  sprintf(buf->release, "%u.%u",
          (unsigned)(LOBYTE(LOWORD(v))), (unsigned)(HIBYTE(LOWORD(v))));
  sprintf(buf->version, "build %u", (unsigned)(HIWORD(v) & 0xffff));
  strcpy(buf->machine, (sizeof(void *) == 8) ? "x86_64" : "i686");
  return 0;
}

int ioctl(int fd, unsigned long request, ...) {
  va_list ap;

  va_start(ap, request);

  if (request == TIOCGWINSZ) {
    struct winsize *ws = va_arg(ap, struct winsize *);
    HANDLE h = GetStdHandle((fd == 0) ? STD_INPUT_HANDLE : STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ci;

    if (ws != NULL && h != INVALID_HANDLE_VALUE &&
        GetConsoleScreenBufferInfo(h, &ci)) {
      ws->ws_col = (unsigned short)(ci.srWindow.Right - ci.srWindow.Left + 1);
      ws->ws_row = (unsigned short)(ci.srWindow.Bottom - ci.srWindow.Top + 1);
      ws->ws_xpixel = 0;
      ws->ws_ypixel = 0;
      va_end(ap);
      return 0;
    }
    errno = ENOTTY;
    va_end(ap);
    return -1;
  }

  if (request == TIOCMGET) {
    int *bits = va_arg(ap, int *);
    if (bits != NULL) {
      *bits = 0;
    }
    va_end(ap);
    return 0;
  }

  va_end(ap);
  errno = ENOTTY;
  return -1;
}

/* ---------------------------------------------------------------------- */
/* System V shared memory emulated with a file-backed mapping             */

struct shm_entry {
  int used;
  key_t key;
  HANDLE hfile;
  HANDLE hmap;
  void *addr;
  size_t size;
  char path[MAX_PATH];
};

static struct shm_entry shm_tab[QM_MAX_SHM];

static void shm_path(key_t key, char *out, size_t out_len) {
  char dir[MAX_PATH];

  win_base_dir(dir, sizeof(dir));
  snprintf(out, out_len, "%s\\shm_%08x.dat", dir, (unsigned int)key);
}

static int shm_lookup(key_t key) {
  int i;

  for (i = 0; i < QM_MAX_SHM; i++) {
    if (shm_tab[i].used && shm_tab[i].key == key) {
      return i;
    }
  }
  return -1;
}

static int shm_free_slot(void) {
  int i;

  for (i = 0; i < QM_MAX_SHM; i++) {
    if (!shm_tab[i].used) {
      return i;
    }
  }
  return -1;
}

static int shm_map_entry(int slot, size_t size) {
  struct shm_entry *e = &shm_tab[slot];
  LARGE_INTEGER li;

  li.QuadPart = (LONGLONG)size;
  if (!SetFilePointerEx(e->hfile, li, NULL, FILE_BEGIN) ||
      !SetEndOfFile(e->hfile)) {
    errno = EIO;
    return -1;
  }

  e->hmap = CreateFileMappingA(e->hfile, win_sa(), PAGE_READWRITE,
                               (DWORD)((size >> 32) & 0xffffffff),
                               (DWORD)(size & 0xffffffff), NULL);
  if (e->hmap == NULL) {
    errno = ENOMEM;
    return -1;
  }

  e->addr = MapViewOfFile(e->hmap, FILE_MAP_ALL_ACCESS, 0, 0, size);
  if (e->addr == NULL) {
    CloseHandle(e->hmap);
    e->hmap = NULL;
    errno = ENOMEM;
    return -1;
  }

  e->size = size;
  return 0;
}

int shmget(key_t key, size_t size, int flags) {
  int slot;
  struct shm_entry *e;
  DWORD disposition = OPEN_EXISTING;
  LARGE_INTEGER fsize;

  if (flags & IPC_CREAT) {
    disposition = OPEN_ALWAYS;
  }

  slot = shm_lookup(key);

  if ((flags & IPC_CREAT) == 0) {
    if (slot < 0) {
      slot = shm_free_slot();
      if (slot < 0) {
        errno = ENOSPC;
        return -1;
      }
      memset(&shm_tab[slot], 0, sizeof(shm_tab[slot]));
      shm_tab[slot].used = 1;
      shm_tab[slot].key = key;
      shm_path(key, shm_tab[slot].path, sizeof(shm_tab[slot].path));
    }
    e = &shm_tab[slot];
    if (e->addr != NULL) {
      return slot;
    }
    e->hfile = CreateFileA(e->path, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, win_sa(),
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (e->hfile == INVALID_HANDLE_VALUE) {
      e->hfile = NULL;
      errno = ENOENT;
      return -1;
    }
    if (!GetFileSizeEx(e->hfile, &fsize)) {
      CloseHandle(e->hfile);
      e->hfile = NULL;
      errno = EIO;
      return -1;
    }
    if (shm_map_entry(slot, (size_t)fsize.QuadPart)) {
      CloseHandle(e->hfile);
      e->hfile = NULL;
      return -1;
    }
    return slot;
  }

  if (slot < 0) {
    slot = shm_free_slot();
    if (slot < 0) {
      errno = ENOSPC;
      return -1;
    }
    memset(&shm_tab[slot], 0, sizeof(shm_tab[slot]));
    shm_tab[slot].used = 1;
    shm_tab[slot].key = key;
    shm_path(key, shm_tab[slot].path, sizeof(shm_tab[slot].path));
  }

  e = &shm_tab[slot];

  if (e->addr != NULL) {
    return slot;
  }

  e->hfile = CreateFileA(e->path, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, win_sa(),
                         disposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if (e->hfile == INVALID_HANDLE_VALUE) {
    e->hfile = NULL;
    if ((flags & IPC_EXCL) && GetLastError() == ERROR_ALREADY_EXISTS) {
      errno = EEXIST;
    } else {
      errno = EIO;
    }
    return -1;
  }

  if ((flags & IPC_EXCL) && GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(e->hfile);
    e->hfile = NULL;
    errno = EEXIST;
    return -1;
  }

  if (size == 0) {
    if (!GetFileSizeEx(e->hfile, &fsize) || fsize.QuadPart == 0) {
      CloseHandle(e->hfile);
      e->hfile = NULL;
      errno = EINVAL;
      return -1;
    }
    size = (size_t)fsize.QuadPart;
  }

  if (shm_map_entry(slot, size)) {
    CloseHandle(e->hfile);
    e->hfile = NULL;
    return -1;
  }

  return slot;
}

void *shmat(int shmid, const void *shmaddr, int shmflg) {
  (void)shmaddr;
  (void)shmflg;

  if (shmid < 0 || shmid >= QM_MAX_SHM || !shm_tab[shmid].used ||
      shm_tab[shmid].addr == NULL) {
    errno = EINVAL;
    return (void *)-1;
  }
  return shm_tab[shmid].addr;
}

int shmdt(const void *shmaddr) {
  int i;

  for (i = 0; i < QM_MAX_SHM; i++) {
    if (shm_tab[i].used && shm_tab[i].addr == shmaddr) {
      if (shm_tab[i].addr != NULL) {
        UnmapViewOfFile(shm_tab[i].addr);
        shm_tab[i].addr = NULL;
      }
      if (shm_tab[i].hmap != NULL) {
        CloseHandle(shm_tab[i].hmap);
        shm_tab[i].hmap = NULL;
      }
      if (shm_tab[i].hfile != NULL) {
        CloseHandle(shm_tab[i].hfile);
        shm_tab[i].hfile = NULL;
      }
      return 0;
    }
  }
  errno = EINVAL;
  return -1;
}

int shmctl(int shmid, int cmd, struct shmid_ds *buf) {
  struct shm_entry *e;
  HANDLE h;

  if (shmid < 0 || shmid >= QM_MAX_SHM || !shm_tab[shmid].used) {
    errno = EINVAL;
    return -1;
  }
  e = &shm_tab[shmid];

  if (cmd == IPC_STAT) {
    if (buf == NULL) {
      errno = EINVAL;
      return -1;
    }
    buf->shm_segsz = e->size;
    if (e->addr != NULL) {
      buf->shm_nattch = 1;
    } else {
      h = CreateFileA(e->path, GENERIC_READ | GENERIC_WRITE, 0, win_sa(),
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
        buf->shm_nattch = 1;
      } else {
        CloseHandle(h);
        buf->shm_nattch = 0;
      }
    }
    return 0;
  }

  if (cmd == IPC_RMID) {
    if (e->addr != NULL) {
      UnmapViewOfFile(e->addr);
      e->addr = NULL;
    }
    if (e->hmap != NULL) {
      CloseHandle(e->hmap);
      e->hmap = NULL;
    }
    if (e->hfile != NULL) {
      CloseHandle(e->hfile);
      e->hfile = NULL;
    }
    DeleteFileA(e->path);
    memset(e, 0, sizeof(*e));
    return 0;
  }

  errno = EINVAL;
  return -1;
}

/* ---------------------------------------------------------------------- */
/* System V semaphores emulated with named Win32 semaphores               */

struct sem_entry {
  int used;
  int id;
  key_t key;
  int nsems;
  char prefix[8];
  HANDLE handles[QM_MAX_SEMAPHORES];
};

static struct sem_entry sem_tab[QM_MAX_SEM];
static int sem_next_id = 1;

static void sem_name(char *out, size_t out_len, const char *prefix, key_t key,
                     int index) {
  snprintf(out, out_len, "%sXQMdata_sem_%08x_%d", prefix,
           (unsigned int)key, index);
}

static struct sem_entry *sem_by_id(int semid) {
  int i;

  for (i = 0; i < QM_MAX_SEM; i++) {
    if (sem_tab[i].used && sem_tab[i].id == semid) {
      return &sem_tab[i];
    }
  }
  return NULL;
}

static struct sem_entry *sem_by_key(key_t key) {
  int i;

  for (i = 0; i < QM_MAX_SEM; i++) {
    if (sem_tab[i].used && sem_tab[i].key == key) {
      return &sem_tab[i];
    }
  }
  return NULL;
}

static HANDLE sem_open_index(struct sem_entry *e, int index) {
  char name[MAX_PATH];
  HANDLE h;

  if (index < 0 || index >= QM_MAX_SEMAPHORES) {
    return NULL;
  }
  if (e->handles[index] != NULL) {
    return e->handles[index];
  }

  sem_name(name, sizeof(name), e->prefix, e->key, index);
  h = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, name);
  if (h == NULL && strcmp(e->prefix, "Global\\") == 0) {
    sem_name(name, sizeof(name), "Local\\", e->key, index);
    h = OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, name);
    if (h != NULL) {
      strcpy(e->prefix, "Local\\");
    }
  }
  e->handles[index] = h;
  return h;
}

int semget(key_t key, int nsems, int flags) {
  struct sem_entry *e;
  char name[MAX_PATH];
  int i;
  int slot = -1;
  int created = 0;
  const char *prefix = "Global\\";

  e = sem_by_key(key);
  if (e != NULL) {
    if (nsems > 0 && (flags & IPC_EXCL)) {
      errno = EEXIST;
      return -1;
    }
    return e->id;
  }

  if (nsems == 0) {
    for (i = 0; i < QM_MAX_SEM; i++) {
      if (!sem_tab[i].used) {
        slot = i;
        break;
      }
    }
    if (slot < 0) {
      errno = ENOSPC;
      return -1;
    }
    e = &sem_tab[slot];
    memset(e, 0, sizeof(*e));
    strcpy(e->prefix, prefix);
    e->key = key;
    for (i = 0; i < QM_MAX_SEMAPHORES; i++) {
      if (sem_open_index(e, i) == NULL) {
        break;
      }
      e->nsems = i + 1;
    }
    if (e->nsems == 0) {
      memset(e, 0, sizeof(*e));
      errno = ENOENT;
      return -1;
    }
    e->used = 1;
    e->id = sem_next_id++;
    return e->id;
  }

  for (i = 0; i < QM_MAX_SEM; i++) {
    if (!sem_tab[i].used) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    errno = ENOSPC;
    return -1;
  }

  e = &sem_tab[slot];
  memset(e, 0, sizeof(*e));
  e->key = key;
  e->nsems = (nsems < QM_MAX_SEMAPHORES) ? nsems : QM_MAX_SEMAPHORES;

  for (i = 0; i < e->nsems; i++) {
    sem_name(name, sizeof(name), prefix, key, i);
    e->handles[i] = CreateSemaphoreA(win_sa(), 1, 1, name);
    if (e->handles[i] == NULL && GetLastError() == ERROR_ACCESS_DENIED) {
      prefix = "Local\\";
      sem_name(name, sizeof(name), prefix, key, i);
      e->handles[i] = CreateSemaphoreA(win_sa(), 1, 1, name);
    }
    if (e->handles[i] == NULL) {
      errno = EACCES;
      goto fail;
    }
    if (i == 0) {
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        created = 0;
        if (flags & IPC_EXCL) {
          errno = EEXIST;
          goto fail;
        }
      } else {
        created = 1;
      }
    }
  }

  e->used = 1;
  e->id = sem_next_id++;
  strcpy(e->prefix, prefix);
  return e->id;

fail:
  for (i = 0; i < e->nsems; i++) {
    if (e->handles[i] != NULL) {
      CloseHandle(e->handles[i]);
      e->handles[i] = NULL;
    }
  }
  memset(e, 0, sizeof(*e));
  (void)created;
  return -1;
}

int semctl(int semid, int semnum, int cmd, ...) {
  va_list ap;
  struct sem_entry *e;
  HANDLE h;

  e = sem_by_id(semid);
  if (e == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (cmd == IPC_RMID) {
    int i;
    for (i = 0; i < QM_MAX_SEMAPHORES; i++) {
      if (e->handles[i] != NULL) {
        CloseHandle(e->handles[i]);
        e->handles[i] = NULL;
      }
    }
    memset(e, 0, sizeof(*e));
    return 0;
  }

  if (cmd == SETVAL || cmd == GETVAL) {
    int val;
    va_start(ap, cmd);
    val = va_arg(ap, int);
    va_end(ap);

    h = sem_open_index(e, semnum);
    if (h == NULL) {
      errno = EINVAL;
      return -1;
    }
    if (cmd == SETVAL) {
      if (val <= 0) {
        WaitForSingleObject(h, 0);
      }
      return 0;
    }
    return val;
  }

  return 0;
}

int semop(int semid, struct sembuf *sops, size_t nsops) {
  struct sem_entry *e;
  size_t i;

  e = sem_by_id(semid);
  if (e == NULL || sops == NULL) {
    errno = EINVAL;
    return -1;
  }

  for (i = 0; i < nsops; i++) {
    HANDLE h = sem_open_index(e, sops[i].sem_num);

    if (h == NULL) {
      errno = EINVAL;
      return -1;
    }
    if (sops[i].sem_op < 0) {
      if (WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0) {
        errno = EINVAL;
        return -1;
      }
    } else if (sops[i].sem_op > 0) {
      if (!ReleaseSemaphore(h, 1, NULL)) {
        errno = EINVAL;
        return -1;
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------------- */
/* Dynamic loading                                                        */

void *dlopen(const char *filename, int flags) {
  (void)flags;
  return (void *)LoadLibraryA(filename);
}

void *dlsym(void *handle, const char *symbol) {
  return (void *)GetProcAddress((HMODULE)handle, symbol);
}

int dlclose(void *handle) {
  return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

char *dlerror(void) {
  static char buf[512];
  DWORD err = GetLastError();

  buf[0] = '\0';
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, err, 0, buf, (DWORD)sizeof(buf), NULL);
  return buf;
}

/* ---------------------------------------------------------------------- */
/* Windows console support for the terminal layer (linuxio.c)              */

/* Return non-zero if fd refers to a Windows console, optionally storing
   the native handle. */
int win_is_console(int fd) {
  HANDLE h;
  DWORD mode;

  if (fd < 0) {
    return 0;
  }
  h = (HANDLE)_get_osfhandle(fd);
  if (h == INVALID_HANDLE_VALUE || h == NULL) {
    return 0;
  }
  return GetConsoleMode(h, &mode) ? 1 : 0;
}

static HANDLE win_console_handle(int fd) {
  HANDLE h;

  if (fd < 0) {
    return NULL;
  }
  h = (HANDLE)_get_osfhandle(fd);
  if (h == INVALID_HANDLE_VALUE) {
    return NULL;
  }
  return h;
}

/* Special key translation state (see win_translate_vk below). Bytes queued
   here are returned by win_console_getch() before reading more input and
   make win_poll_fd() report the console as ready. */
static char win_key_pending[32];
static int win_key_pending_len = 0;
static int win_key_pending_pos = 0;

/* Poll a console handle: return 1 when at least one character-producing
   key event is queued, 0 on timeout. Other events (focus, mouse, key-up,
   non-ASCII keys) are discarded so that they do not cause a busy loop. */
static int win_poll_console(HANDLE h, int timeout) {
  DWORD waited = 0;
  const DWORD slice = 50;
  DWORD limit = (timeout < 0) ? INFINITE : (DWORD)timeout;

  for (;;) {
    INPUT_RECORD recs[16];
    DWORD count = 0;
    DWORD i;
    int useful = 0;
    int junk = 0;

    if (PeekConsoleInputA(h, recs, 16, &count)) {
      for (i = 0; i < count; i++) {
        /* Any key-down event is useful: win_console_getch() will either
           return its character or translate it to a terminfo sequence. */
        if (recs[i].EventType == KEY_EVENT &&
            recs[i].Event.KeyEvent.bKeyDown) {
          useful = 1;
        } else {
          junk = 1;
        }
      }
    }

    if (useful) {
      return 1;
    }

    if (junk) {
      /* Consume the uninteresting events so the next peek sees fresh data */
      DWORD dummy = 0;
      ReadConsoleInputA(h, recs, 16, &dummy);
      continue;
    }

    if (limit != INFINITE && waited >= limit) {
      return 0;
    }

    {
      DWORD step = (limit == INFINITE) ? slice
                                       : ((limit - waited > slice) ? slice
                                                                   : limit - waited);
      if (step == 0) {
        return 0;
      }
      WaitForSingleObject(h, step);
      waited += step;
    }
  }
}

/* Winsock must be initialised in every engine process that touches
   sockets (network sessions via qm -N, QMClient, etc.). Run it as a
   constructor so no call site has to remember. */
__attribute__((constructor)) static void win_wsa_init(void) {
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
}

/* ---------------------------------------------------------------------- */
/* Network session socket (qm -N<socket>, telnet via qmteld)               */
/*                                                                         */
/* ReadFile/WriteFile do NOT work on sockets under this CRT, so the        */
/* accepted socket cannot simply be wrapped with _open_osfhandle().        */
/* Instead, qm -N registers its socket here and our read()/write()         */
/* overrides translate I/O on fds 0/1 into recv()/send().                  */

static SOCKET win_net_socket = INVALID_SOCKET;

void win_register_net_socket(uintptr_t s) {
  win_net_socket = (SOCKET)s;
}

uintptr_t win_socket_for_fd(int fd) {
  if ((fd == 0 || fd == 1) && win_net_socket != INVALID_SOCKET) {
    return (uintptr_t)win_net_socket;
  }
  return (uintptr_t)INVALID_SOCKET;
}

/* read()/write() overrides: shadow the CRT versions for the whole engine.
   Only fds 0/1 are intercepted and only when a network socket is active;
   everything else is passed through to the CRT. */
int read(int fd, void *buf, unsigned int count) {
  if (fd == 0 && win_net_socket != INVALID_SOCKET) {
    int n = recv(win_net_socket, (char *)buf, (int)count, 0);
    if (n == SOCKET_ERROR) {
      errno = EIO;
      return -1;
    }
    return n;
  }
  return _read(fd, buf, count);
}

int write(int fd, const void *buf, unsigned int count) {
  /* On a network session, fds 0 and 1 are the same socket (xinetd model),
     so writes to either must go to the socket. The QMClient server sends
     its initial Ack on fd 0. */
  if ((fd == 0 || fd == 1) && win_net_socket != INVALID_SOCKET) {
    int n = send(win_net_socket, (const char *)buf, (int)count, 0);
    if (n == SOCKET_ERROR) {
      errno = EIO;
      return -1;
    }
    return n;
  }
  return _write(fd, buf, count);
}

/* poll() replacement used by qmpoll() on Windows. WSAPoll() only accepts
   sockets, so console handles, pipes and files need their own logic. */
int win_poll_fd(int fd, int timeout) {
  HANDLE h;
  DWORD ftype;
  SOCKET ws;

  /* Bytes queued by a special key translation are always "input" */
  if (win_key_pending_pos < win_key_pending_len) {
    return 1;
  }

  /* Registered network session socket? */
  ws = (SOCKET)win_socket_for_fd(fd);
  if (ws != INVALID_SOCKET) {
    WSAPOLLFD pfd;
    int r;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = ws;
    pfd.events = POLLIN;

    r = WSAPoll(&pfd, 1, timeout);
    if (r == SOCKET_ERROR) {
      errno = EBADF;
      return -1;
    }
    return r;
  }

  h = win_console_handle(fd);

  if (h != NULL) {
    DWORD mode;
    if (GetConsoleMode(h, &mode)) {
      return win_poll_console(h, timeout);
    }

    ftype = GetFileType(h);
    if (ftype == FILE_TYPE_DISK) {
      return 1; /* Regular files are always readable */
    }
    if (ftype == FILE_TYPE_PIPE) {
      DWORD waited = 0;
      const DWORD slice = 50;
      DWORD limit = (timeout < 0) ? INFINITE : (DWORD)timeout;

      for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL)) {
          return 1; /* Broken pipe: let the read return EOF, like poll() */
        }
        if (avail > 0) {
          return 1;
        }
        if (limit != INFINITE && waited >= limit) {
          return 0;
        }
        Sleep((limit == INFINITE) ? slice
                                  : ((limit - waited > slice) ? slice
                                                              : limit - waited));
        waited += slice;
      }
    }
  }

  /* Assume it is a socket */
  {
    WSAPOLLFD pfd;
    int r;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = (SOCKET)(intptr_t)fd;
    pfd.events = POLLIN;

    r = WSAPoll(&pfd, 1, timeout);
    if (r == SOCKET_ERROR) {
      errno = EBADF;
      return -1;
    }
    return r;
  }
}

/* ---------------------------------------------------------------------- */
/* Special key translation: Windows virtual keys -> terminfo sequences     */

/* qmtgetstr() lives in qmtermlb.c, which is only linked into qm.exe.
   Provide a weak fallback so that qmlnxd.exe (no terminal library) links;
   the real implementation overrides this one when present. */
__attribute__((weak)) char *qmtgetstr(char *id) {
  (void)id;
  return NULL;
}

static const struct {
  WORD vk;
  const char *cap;
} win_vk_keys[] = {
    {VK_LEFT, "kcub1"},  {VK_RIGHT, "kcuf1"}, {VK_UP, "kcuu1"},
    {VK_DOWN, "kcud1"},  {VK_PRIOR, "kpp"},   {VK_NEXT, "knp"},
    {VK_HOME, "khome"},  {VK_END, "kend"},    {VK_INSERT, "kich1"},
    {VK_DELETE, "kdch1"},
    {VK_F1, "kf1"},      {VK_F2, "kf2"},      {VK_F3, "kf3"},
    {VK_F4, "kf4"},      {VK_F5, "kf5"},      {VK_F6, "kf6"},
    {VK_F7, "kf7"},      {VK_F8, "kf8"},      {VK_F9, "kf9"},
    {VK_F10, "kf10"},    {VK_F11, "kf11"},    {VK_F12, "kf12"},
};

/* Queue the terminfo string for a capability so that subsequent getch
   calls return it one byte at a time. Returns non-zero if queued. */
static int win_inject_keyseq(const char *cap) {
  char *s;
  size_t n;

  if (cap == NULL) {
    return 0;
  }
  s = qmtgetstr((char *)cap);
  if (s == NULL || s == (char *)-1) {
    return 0; /* Capability not defined for this terminal type */
  }

  n = strlen(s);
  if (n == 0 || n > sizeof(win_key_pending)) {
    return 0;
  }

  memcpy(win_key_pending, s, n);
  win_key_pending_len = (int)n;
  win_key_pending_pos = 0;
  return 1;
}

static int win_translate_vk(const KEY_EVENT_RECORD *kev) {
  unsigned int i;

  /* Shift+Tab -> backtab */
  if (kev->wVirtualKeyCode == VK_TAB &&
      (kev->dwControlKeyState & SHIFT_PRESSED)) {
    return win_inject_keyseq("kcbt");
  }

  for (i = 0; i < sizeof(win_vk_keys) / sizeof(win_vk_keys[0]); i++) {
    if (win_vk_keys[i].vk == kev->wVirtualKeyCode) {
      return win_inject_keyseq(win_vk_keys[i].cap);
    }
  }
  return 0;
}

/* Blocking read of a single character from the console. Key-up events,
   non-ASCII keys (arrows, function keys) and other console events are
   discarded. Returns the character (0-255) or -1 on error. */
int win_console_getch(void) {
  HANDLE h = win_console_handle(0);
  INPUT_RECORD rec;
  DWORD count;

  if (h == NULL) {
    return -1;
  }

  for (;;) {
    /* Drain any pending bytes from a special key translation first */
    if (win_key_pending_pos < win_key_pending_len) {
      return (int)(unsigned char)win_key_pending[win_key_pending_pos++];
    }
    win_key_pending_len = 0;
    win_key_pending_pos = 0;

    if (!ReadConsoleInputA(h, &rec, 1, &count)) {
      return -1;
    }
    if (count == 0 || rec.EventType != KEY_EVENT ||
        !rec.Event.KeyEvent.bKeyDown) {
      continue;
    }

    if (rec.Event.KeyEvent.uChar.AsciiChar != 0) {
      /* Shift+Tab may still report an ASCII tab */
      if (rec.Event.KeyEvent.wVirtualKeyCode == VK_TAB &&
          (rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) &&
          win_inject_keyseq("kcbt")) {
        continue;
      }
      return (int)(unsigned char)rec.Event.KeyEvent.uChar.AsciiChar;
    }

    /* Non-ASCII key: try to translate it to a terminfo key sequence */
    if (win_translate_vk(&rec.Event.KeyEvent)) {
      continue; /* Loop returns the first pending byte */
    }
    /* Otherwise discard the event */
  }
}

/* ---------------------------------------------------------------------- */
/* Terminal attributes mapped to Win32 console modes                      */

int tcgetattr(int fd, struct termios *termios_p) {
  HANDLE h;
  DWORD mode;

  if (termios_p == NULL) {
    errno = EINVAL;
    return -1;
  }
  memset(termios_p, 0, sizeof(*termios_p));

  h = win_console_handle(fd);
  if (h != NULL && GetConsoleMode(h, &mode)) {
    if (mode & ENABLE_LINE_INPUT)
      termios_p->c_lflag |= ICANON;
    if (mode & ENABLE_ECHO_INPUT)
      termios_p->c_lflag |= ECHO;
    if (mode & ENABLE_PROCESSED_INPUT)
      termios_p->c_lflag |= ISIG;
  } else {
    /* Not a console: report a cooked mode so callers behave sanely */
    termios_p->c_lflag = ICANON | ECHO | ISIG;
  }
  return 0;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
  HANDLE h;
  DWORD mode;

  (void)optional_actions;

  if (termios_p == NULL) {
    errno = EINVAL;
    return -1;
  }

  h = win_console_handle(fd);
  if (h == NULL || !GetConsoleMode(h, &mode)) {
    return 0; /* Not a console: nothing to do */
  }

  mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT |
            ENABLE_QUICK_EDIT_MODE);
  if (termios_p->c_lflag & ICANON)
    mode |= ENABLE_LINE_INPUT;
  if (termios_p->c_lflag & ECHO)
    mode |= ENABLE_ECHO_INPUT;
  if (termios_p->c_lflag & ISIG)
    mode |= ENABLE_PROCESSED_INPUT;

  SetConsoleMode(h, mode | ENABLE_EXTENDED_FLAGS);
  return 0;
}

int tcdrain(int fd) {
  (void)fd;
  return 0;
}

int tcflush(int fd, int queue_selector) {
  (void)fd;
  (void)queue_selector;
  return 0;
}

int cfmakeraw(struct termios *termios_p) {
  if (termios_p != NULL) {
    memset(termios_p, 0, sizeof(*termios_p));
  }
  return 0;
}

speed_t cfgetospeed(const struct termios *termios_p) {
  (void)termios_p;
  return B9600;
}

speed_t cfgetispeed(const struct termios *termios_p) {
  (void)termios_p;
  return B9600;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
  (void)termios_p;
  (void)speed;
  return 0;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
  (void)termios_p;
  (void)speed;
  return 0;
}

/* ---------------------------------------------------------------------- */
/* Password / account placeholders (Windows uses QM's own user file)      */

char *crypt(const char *key, const char *salt) {
  static char empty[1] = "";
  (void)key;
  (void)salt;
  return empty;
}

struct passwd *getpwnam(const char *name) {
  (void)name;
  return NULL;
}

struct passwd *getpwuid(unsigned int uid) {
  (void)uid;
  return NULL;
}
