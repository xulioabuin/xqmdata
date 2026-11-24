#ifndef QM_WIN32_H
#define QM_WIN32_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <unistd.h>

#include <sys/types.h>
#include <signal.h>

#include <sys/utsname.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#ifndef SIGTRAP
#define SIGTRAP 5
#endif
#ifndef SIGBUS
#define SIGBUS 7
#endif
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGUSR1
#define SIGUSR1 10
#endif
#ifndef SIGUSR2
#define SIGUSR2 12
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGALRM
#define SIGALRM 14
#endif
#ifndef SIGCHLD
#define SIGCHLD 17
#endif
#ifndef SIGIO
#define SIGIO 29
#endif

#ifndef EAI_SYSTEM
#define EAI_SYSTEM 70001
#endif

#ifndef F_GETFD
#define F_GETFD 1
#endif
#ifndef F_SETFD
#define F_SETFD 2
#endif
#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef F_SETOWN
#define F_SETOWN 8
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif
#ifndef O_ASYNC
#define O_ASYNC 0x2000
#endif
#ifndef O_NOCTTY
#define O_NOCTTY 0x8000
#endif

#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

typedef int sigset_t;
#ifndef SIG_BLOCK
#define SIG_BLOCK 0
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

static __inline int sigemptyset(sigset_t *s) {
  *s = 0;
  return 0;
}
static __inline int sigfillset(sigset_t *s) {
  *s = ~0;
  return 0;
}
static __inline int sigaddset(sigset_t *s, int signo) {
  (void)signo;
  *s |= 1;
  return 0;
}
static __inline int sigdelset(sigset_t *s, int signo) {
  (void)signo;
  *s &= ~1;
  return 0;
}
static __inline int sigprocmask(int how, const sigset_t *set, sigset_t *old) {
  (void)how;
  (void)set;
  (void)old;
  return 0;
}

unsigned int getuid(void);
unsigned int geteuid(void);
unsigned int getgid(void);
unsigned int getegid(void);

int qm_gethostname(char *name, int len);
int fsync(int fd);
int kill(pid_t pid, int sig);
int fcntl(int fd, int cmd, ...);
char *ttyname(int fd);
int qm_spawn_detached(const char *exe, const char *args, int *pid_out);

/* Windows console support for the terminal layer (linuxio.c) */
int win_is_console(int fd);
int win_poll_fd(int fd, int timeout);
int win_console_getch(void);

/* Network session support (qm -N<socket>, telnet via qmteld) */
void win_register_net_socket(uintptr_t s);
uintptr_t win_socket_for_fd(int fd);

#ifdef __cplusplus
}
#endif

#endif
