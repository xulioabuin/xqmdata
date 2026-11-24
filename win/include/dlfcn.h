#ifndef QM_WIN_DLFCN_H
#define QM_WIN_DLFCN_H

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_LAZY   0
#define RTLD_NOW    0
#define RTLD_LOCAL  0
#define RTLD_GLOBAL 0

void *dlopen(const char *filename, int flags);
void *dlsym(void *handle, const char *symbol);
int dlclose(void *handle);
char *dlerror(void);

#ifdef __cplusplus
}
#endif

#endif
