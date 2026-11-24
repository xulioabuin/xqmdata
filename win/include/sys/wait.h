#ifndef QM_WIN_SYS_WAIT_H
#define QM_WIN_SYS_WAIT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WNOHANG
#define WNOHANG 1
#endif
#ifndef WUNTRACED
#define WUNTRACED 2
#endif

#ifndef WIFEXITED
#define WIFEXITED(status) 1
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(status) (status)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(status) 0
#endif
#ifndef WTERMSIG
#define WTERMSIG(status) 0
#endif
#ifndef WIFSTOPPED
#define WIFSTOPPED(status) 0
#endif

pid_t waitpid(pid_t pid, int *status, int options);

#ifdef __cplusplus
}
#endif

#endif
