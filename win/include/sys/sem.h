#ifndef QM_WIN_SYS_SEM_H
#define QM_WIN_SYS_SEM_H

#include <sys/types.h>
#include <sys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sembuf {
  unsigned short sem_num;
  short sem_op;
  short sem_flg;
};

#ifndef SEM_UNDO
#define SEM_UNDO 0x1000
#endif

int semget(key_t key, int nsems, int flags);
int semctl(int semid, int semnum, int cmd, ...);
int semop(int semid, struct sembuf *sops, size_t nsops);

#ifdef __cplusplus
}
#endif

#endif
