#ifndef QM_WIN_SYS_IOCTL_H
#define QM_WIN_SYS_IOCTL_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
};

#define TIOCGWINSZ 0x5413
#define TIOCMGET   0x5415
#define TIOCMSET   0x5418

#define TIOCM_LE   0x001
#define TIOCM_DTR  0x002
#define TIOCM_RTS  0x004
#define TIOCM_ST   0x008
#define TIOCM_SR   0x010
#define TIOCM_CTS  0x020
#define TIOCM_CAR  0x040
#define TIOCM_RNG  0x080
#define TIOCM_DSR  0x100

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif
