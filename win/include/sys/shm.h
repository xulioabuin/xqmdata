#ifndef QM_WIN_SYS_SHM_H
#define QM_WIN_SYS_SHM_H

#include <sys/types.h>
#include <sys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct shmid_ds {
  size_t shm_nattch;
  size_t shm_segsz;
};

#define SHM_RDONLY 010000
#define SHM_RND    020000

int shmget(key_t key, size_t size, int flags);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);

#ifdef __cplusplus
}
#endif

#endif
