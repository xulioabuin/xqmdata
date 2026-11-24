#ifndef QM_WIN_CRYPT_H
#define QM_WIN_CRYPT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Provided by winipc.c.  Windows does not use /etc/shadow for QM user
   authentication, but the symbol must exist for the POSIX code paths. */
char *crypt(const char *key, const char *salt);

#ifdef __cplusplus
}
#endif

#endif
