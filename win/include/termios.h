#ifndef QM_WIN_TERMIOS_H
#define QM_WIN_TERMIOS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MinGW-w64 has no termios.  These definitions exist so that the POSIX
   terminal handling code compiles.  The real console handling on Windows
   is done with the Win32 console API in linuxio.c. */

struct termios {
  unsigned int c_iflag;
  unsigned int c_oflag;
  unsigned int c_cflag;
  unsigned int c_lflag;
  unsigned char c_cc[32];
  unsigned int c_ispeed;
  unsigned int c_ospeed;
};

/* c_cc indices */
#define VEOF   0
#define VEOL   1
#define VERASE 2
#define VINTR  3
#define VKILL  4
#define VQUIT  5
#define VSTART 6
#define VSTOP  7
#define VSUSP  8
#define VMIN   9
#define VTIME  10

/* c_iflag */
#define IGNBRK 0x0001
#define BRKINT 0x0002
#define IGNPAR 0x0004
#define PARMRK 0x0008
#define INPCK  0x0010
#define ISTRIP 0x0020
#define INLCR  0x0040
#define IGNCR  0x0080
#define ICRNL  0x0100
#define IXON   0x0200
#define IXOFF  0x0400
#define IXANY  0x0800

/* c_oflag */
#define OPOST  0x0001

/* c_cflag */
#define CSIZE  0x0030
#define CS5    0x0000
#define CS6    0x0010
#define CS7    0x0020
#define CS8    0x0030
#define CSTOPB 0x0040
#define CREAD  0x0080
#define PARENB 0x0100
#define PARODD 0x0200
#define HUPCL  0x0400

/* c_lflag */
#define ISIG   0x0001
#define ICANON 0x0002
#define ECHO   0x0008
#define ECHOE  0x0010
#define ECHOK  0x0020
#define ECHONL 0x0040
#define NOFLSH 0x0080
#define IEXTEN 0x0100

#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

typedef unsigned int speed_t;

#define B0      0
#define B50     1
#define B75     2
#define B110    3
#define B134    4
#define B150    5
#define B200    6
#define B300    7
#define B600    8
#define B1200   9
#define B1800   10
#define B2400   11
#define B4800   12
#define B9600   13
#define B19200  14
#define B38400  15
#define B57600  0x1001
#define B115200 0x1002
#define B230400 0x1003

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int cfmakeraw(struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
speed_t cfgetispeed(const struct termios *termios_p);
int cfsetospeed(struct termios *termios_p, speed_t speed);
int cfsetispeed(struct termios *termios_p, speed_t speed);

#ifdef __cplusplus
}
#endif

#endif
