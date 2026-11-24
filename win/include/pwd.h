#ifndef QM_WIN_PWD_H
#define QM_WIN_PWD_H

#ifdef __cplusplus
extern "C" {
#endif

struct passwd {
  char *pw_name;
  char *pw_passwd;
  unsigned int pw_uid;
  unsigned int pw_gid;
  char *pw_gecos;
  char *pw_dir;
  char *pw_shell;
};

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(unsigned int uid);

#ifdef __cplusplus
}
#endif

#endif
