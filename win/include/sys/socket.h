#ifndef QM_WIN_SYS_SOCKET_H
#define QM_WIN_SYS_SOCKET_H

/* MinGW-w64 compatibility shim: expose Winsock2 as the BSD socket API. */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#endif
