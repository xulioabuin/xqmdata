/* XQMCLILIB_W.C
 *
 * Unicode (wchar_t) wrappers over the ANSI QMClient C library (qmclilib.c).
 *
 * The GPL sources only ship the ANSI (char*) client API. The proprietary
 * OpenQM library also exports "*W" variants used by modern clients
 * (qmclient.py, .NET). This file provides them for the XQMdata build.
 *
 * wchar_t on MinGW/Windows is 16 bits, matching WCHAR, so the Win32
 * MultiByteToWideChar / WideCharToMultiByte APIs are used for conversion.
 * Strings are converted with CP_ACP (the system ANSI code page), which is
 * the byte encoding the engine expects for data.
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define DLLEntry __declspec(dllexport)

/* ------------------------------------------------------------------ */
/* Declarations of the ANSI functions (qmclilib.c)                     */

extern void QMCall(char *subrname, short int argc, ...);
extern int QMCallx(char *subrname, short int argc, ...);
extern char *QMChange(char *src, char *old, char *new, int occurrences,
                      int start);
extern void QMClearSelect(int listno);
extern void QMClose(int fno);
extern int QMConnect(char *host, int port, char *username, char *password,
                     char *account);
extern int QMConnected(void);
extern int QMConnectLocal(char *account);
extern int QMDcount(char *src, char *delim_str);
extern char *QMDel(char *src, int fno, int vno, int svno);
extern void QMDelete(int fno, char *id);
extern void QMDeleteu(int fno, char *id);
extern void QMDisconnect(void);
extern void QMDisconnectAll(void);
extern void QMEndCommand(void);
extern int QMEnterPackage(char *name);
extern int QMExitPackage(char *name);
extern char *QMError(void);
extern char *QMExecute(char *cmnd, int *err);
extern char *QMExtract(char *src, int fno, int vno, int svno);
extern char *QMField(char *src, char *delim, int first, int occurrences);
extern void QMFree(void *p);
extern int QMGetSession(void);
extern char *QMIns(char *src, int fno, int vno, int svno, char *new);
extern int QMLocate(char *item, char *src, int fno, int vno, int svno,
                    int *pos, char *order);
extern int QMLogto(char *account_name);
extern void QMMarkMapping(short int fno, short int state);
extern int QMMatch(char *str, char *pattern);
extern char *QMMatchfield(char *str, char *pattern, int component);
extern int QMOpen(char *filename);
extern char *QMRead(int fno, char *id, int *err);
extern char *QMReadl(int fno, char *id, int wait, int *err);
extern char *QMReadList(int listno);
extern char *QMReadNext(short int listno);
extern char *QMReadu(int fno, char *id, int wait, int *err);
extern void QMRecordlock(int fno, char *id, int update_lock, int wait);
extern void QMRelease(int fno, char *id);
extern char *QMReplace(char *src, int fno, int vno, int svno, char *new);
extern char *QMRespond(char *response, int *err);
extern void QMSelect(int fno, int listno);
extern void QMSelectIndex(short int fno, char *index_name, char *index_value,
                          short int listno);
extern char *QMSelectLeft(short int fno, char *index_name, short int listno);
extern char *QMSelectRight(short int fno, char *index_name, short int listno);
extern void QMSetLeft(short int fno, char *index_name);
extern void QMSetRight(short int fno, char *index_name);
extern int QMSetSession(int idx);
extern int QMStatus(void);
extern void QMWrite(int fno, char *id, char *data);
extern void QMWriteu(int fno, char *id, char *data);

/* ------------------------------------------------------------------ */
/* Conversion helpers                                                  */

/* wchar_t* -> malloc'd char* (CP_ACP). NULL in, NULL out. */
static char *w2c(const wchar_t *w) {
  int n;
  char *s;

  if (w == NULL) {
    return NULL;
  }
  n = WideCharToMultiByte(CP_ACP, 0, w, -1, NULL, 0, NULL, NULL);
  if (n <= 0) {
    return NULL;
  }
  s = (char *)malloc((size_t)n);
  if (s != NULL) {
    WideCharToMultiByte(CP_ACP, 0, w, -1, s, n, NULL, NULL);
  }
  return s;
}

/* char* -> malloc'd wchar_t* (CP_ACP). NULL in, NULL out. */
static wchar_t *c2w(const char *c) {
  int n;
  wchar_t *w;

  if (c == NULL) {
    return NULL;
  }
  n = MultiByteToWideChar(CP_ACP, 0, c, -1, NULL, 0);
  if (n <= 0) {
    return NULL;
  }
  w = (wchar_t *)malloc((size_t)n * sizeof(wchar_t));
  if (w != NULL) {
    MultiByteToWideChar(CP_ACP, 0, c, -1, w, n);
  }
  return w;
}

/* Call ANSI function returning malloc'd char*, convert result to a
   malloc'd wchar_t*, freeing the intermediate. */
#define WRAP_STR_RESULT(expr)                 \
  do {                                        \
    char *_r = (expr);                        \
    wchar_t *_w = c2w(_r);                    \
    if (_r != NULL) {                         \
      free(_r);                               \
    }                                         \
    return _w;                                \
  } while (0)

/* ------------------------------------------------------------------ */
/* Subroutine calls                                                    */

/* Shared logic for QMCallW / QMCallxW: converts up to 20 wchar_t* args,
   invokes the variadic ANSI function, then converts any updated argument
   strings back into the caller's wchar_t buffers. */
static int call_common(const wchar_t *subrname, short int argc, va_list ap,
                       int use_x) {
  char *subr_c;
  char *cargs[20];
  wchar_t *wargs[20];
  int i;
  int status = 0;

  if (argc < 0 || argc > 20) {
    return 0;
  }

  subr_c = w2c(subrname);
  if (subr_c == NULL) {
    return 0;
  }

  for (i = 0; i < argc; i++) {
    wargs[i] = va_arg(ap, wchar_t *);
    cargs[i] = w2c(wargs[i]);
    if (cargs[i] == NULL) {
      cargs[i] = (char *)calloc(1, 1);
    }
  }

  if (use_x) {
    status = QMCallx(subr_c, argc, cargs[0], cargs[1], cargs[2], cargs[3],
                     cargs[4], cargs[5], cargs[6], cargs[7], cargs[8],
                     cargs[9], cargs[10], cargs[11], cargs[12], cargs[13],
                     cargs[14], cargs[15], cargs[16], cargs[17], cargs[18],
                     cargs[19]);
  } else {
    QMCall(subr_c, argc, cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
           cargs[5], cargs[6], cargs[7], cargs[8], cargs[9], cargs[10],
           cargs[11], cargs[12], cargs[13], cargs[14], cargs[15], cargs[16],
           cargs[17], cargs[18], cargs[19]);
  }

  /* Convert any updated argument strings back into the caller's buffers */
  for (i = 0; i < argc; i++) {
    if (cargs[i] != NULL && wargs[i] != NULL && cargs[i][0] != '\0') {
      wchar_t *back = c2w(cargs[i]);
      if (back != NULL) {
        wcscpy(wargs[i], back);
        free(back);
      }
    }
    free(cargs[i]);
  }
  free(subr_c);
  return status;
}

void DLLEntry QMCallW(wchar_t *subrname, short int argc, ...) {
  va_list ap;
  va_start(ap, argc);
  (void)call_common(subrname, argc, ap, 0);
  va_end(ap);
}

int DLLEntry QMCallxW(wchar_t *subrname, short int argc, ...) {
  int status;
  va_list ap;
  va_start(ap, argc);
  status = call_common(subrname, argc, ap, 1);
  va_end(ap);
  return status;
}

/* ------------------------------------------------------------------ */
/* String / dynamic array handling                                     */

wchar_t *DLLEntry QMChangeW(wchar_t *src, wchar_t *old, wchar_t *newstr,
                            int occurrences, int start) {
  char *s = w2c(src);
  char *o = w2c(old);
  char *n = w2c(newstr);
  WRAP_STR_RESULT(QMChange(s, o, n, occurrences, start));
}

int DLLEntry QMDcountW(wchar_t *src, wchar_t *delim_str) {
  char *s = w2c(src);
  char *d = w2c(delim_str);
  int r = QMDcount(s, d);
  free(s);
  free(d);
  return r;
}

wchar_t *DLLEntry QMDelW(wchar_t *src, int fno, int vno, int svno) {
  char *s = w2c(src);
  WRAP_STR_RESULT(QMDel(s, fno, vno, svno));
}

void DLLEntry QMDeleteW(int fno, wchar_t *id) {
  char *i = w2c(id);
  QMDelete(fno, i);
  free(i);
}

void DLLEntry QMDeleteuW(int fno, wchar_t *id) {
  char *i = w2c(id);
  QMDeleteu(fno, i);
  free(i);
}

wchar_t *DLLEntry QMExtractW(wchar_t *src, int fno, int vno, int svno) {
  char *s = w2c(src);
  WRAP_STR_RESULT(QMExtract(s, fno, vno, svno));
}

wchar_t *DLLEntry QMFieldW(wchar_t *src, wchar_t *delim, int first,
                           int occurrences) {
  char *s = w2c(src);
  char *d = w2c(delim);
  WRAP_STR_RESULT(QMField(s, d, first, occurrences));
}

wchar_t *DLLEntry QMInsW(wchar_t *src, int fno, int vno, int svno,
                         wchar_t *newstr) {
  char *s = w2c(src);
  char *n = w2c(newstr);
  WRAP_STR_RESULT(QMIns(s, fno, vno, svno, n));
}

int DLLEntry QMLocateW(wchar_t *item, wchar_t *src, int fno, int vno,
                       int svno, int *pos, wchar_t *order) {
  char *it = w2c(item);
  char *s = w2c(src);
  char *o = w2c(order);
  int r = QMLocate(it, s, fno, vno, svno, pos, o);
  free(it);
  free(s);
  free(o);
  return r;
}

int DLLEntry QMMatchW(wchar_t *str, wchar_t *pattern) {
  char *s = w2c(str);
  char *p = w2c(pattern);
  int r = QMMatch(s, p);
  free(s);
  free(p);
  return r;
}

wchar_t *DLLEntry QMMatchfieldW(wchar_t *str, wchar_t *pattern,
                                int component) {
  char *s = w2c(str);
  char *p = w2c(pattern);
  WRAP_STR_RESULT(QMMatchfield(s, p, component));
}

wchar_t *DLLEntry QMReplaceW(wchar_t *src, int fno, int vno, int svno,
                             wchar_t *newstr) {
  char *s = w2c(src);
  char *n = w2c(newstr);
  WRAP_STR_RESULT(QMReplace(s, fno, vno, svno, n));
}

/* ------------------------------------------------------------------ */
/* Session management                                                  */

int DLLEntry QMConnectW(wchar_t *host, int port, wchar_t *username,
                        wchar_t *password, wchar_t *account) {
  char *h = w2c(host);
  char *u = w2c(username);
  char *p = w2c(password);
  char *a = w2c(account);
  int r = QMConnect(h, port, u, p, a);
  free(h);
  free(u);
  free(p);
  free(a);
  return r;
}

int DLLEntry QMConnectLocalW(wchar_t *account) {
  char *a = w2c(account);
  int r = QMConnectLocal(a);
  free(a);
  return r;
}

int DLLEntry QMLogtoW(wchar_t *account) {
  char *a = w2c(account);
  int r = QMLogto(a);
  free(a);
  return r;
}

int DLLEntry QMEnterPackageW(wchar_t *name) {
  char *n = w2c(name);
  int r = QMEnterPackage(n);
  free(n);
  return r;
}

int DLLEntry QMExitPackageW(wchar_t *name) {
  char *n = w2c(name);
  int r = QMExitPackage(n);
  free(n);
  return r;
}

wchar_t *DLLEntry QMErrorW(void) {
  /* QMError() returns a static buffer; do not free it */
  return c2w(QMError());
}

/* ------------------------------------------------------------------ */
/* Command execution                                                   */

wchar_t *DLLEntry QMExecuteW(wchar_t *cmnd, int *err) {
  char *c = w2c(cmnd);
  WRAP_STR_RESULT(QMExecute(c, err));
}

wchar_t *DLLEntry QMRespondW(wchar_t *response, int *err) {
  char *r = w2c(response);
  WRAP_STR_RESULT(QMRespond(r, err));
}

/* ------------------------------------------------------------------ */
/* File handling                                                       */

int DLLEntry QMOpenW(wchar_t *filename) {
  char *f = w2c(filename);
  int r = QMOpen(f);
  free(f);
  return r;
}

wchar_t *DLLEntry QMReadW(int fno, wchar_t *id, int *err) {
  char *i = w2c(id);
  WRAP_STR_RESULT(QMRead(fno, i, err));
}

wchar_t *DLLEntry QMReadlW(int fno, wchar_t *id, int wait, int *err) {
  char *i = w2c(id);
  WRAP_STR_RESULT(QMReadl(fno, i, wait, err));
}

wchar_t *DLLEntry QMReaduW(int fno, wchar_t *id, int wait, int *err) {
  char *i = w2c(id);
  WRAP_STR_RESULT(QMReadu(fno, i, wait, err));
}

wchar_t *DLLEntry QMReadListW(int listno) {
  WRAP_STR_RESULT(QMReadList(listno));
}

wchar_t *DLLEntry QMReadNextW(short int listno) {
  WRAP_STR_RESULT(QMReadNext(listno));
}

void DLLEntry QMRecordlockW(int fno, wchar_t *id, int update_lock, int wait) {
  char *i = w2c(id);
  QMRecordlock(fno, i, update_lock, wait);
  free(i);
}

void DLLEntry QMReleaseW(int fno, wchar_t *id) {
  char *i = w2c(id);
  QMRelease(fno, i);
  free(i);
}

void DLLEntry QMWriteW(int fno, wchar_t *id, wchar_t *data) {
  char *i = w2c(id);
  char *d = w2c(data);
  QMWrite(fno, i, d);
  free(i);
  free(d);
}

void DLLEntry QMWriteuW(int fno, wchar_t *id, wchar_t *data) {
  char *i = w2c(id);
  char *d = w2c(data);
  QMWriteu(fno, i, d);
  free(i);
  free(d);
}

/* ------------------------------------------------------------------ */
/* Select / index handling                                             */

void DLLEntry QMSelectIndexW(short int fno, wchar_t *index_name,
                             wchar_t *index_value, short int listno) {
  char *n = w2c(index_name);
  char *v = w2c(index_value);
  QMSelectIndex(fno, n, v, listno);
  free(n);
  free(v);
}

wchar_t *DLLEntry QMSelectLeftW(short int fno, wchar_t *index_name,
                                short int listno) {
  char *n = w2c(index_name);
  WRAP_STR_RESULT(QMSelectLeft(fno, n, listno));
}

wchar_t *DLLEntry QMSelectRightW(short int fno, wchar_t *index_name,
                                 short int listno) {
  char *n = w2c(index_name);
  WRAP_STR_RESULT(QMSelectRight(fno, n, listno));
}

void DLLEntry QMSetLeftW(short int fno, wchar_t *index_name) {
  char *n = w2c(index_name);
  QMSetLeft(fno, n);
  free(n);
}

void DLLEntry QMSetRightW(short int fno, wchar_t *index_name) {
  char *n = w2c(index_name);
  QMSetRight(fno, n);
  free(n);
}

/* END-CODE */
