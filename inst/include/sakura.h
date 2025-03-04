// sakura - public header file -------------------------------------------------

#ifndef SAKURA_H
#define SAKURA_H

#include <R.h>
#include <Rinternals.h>

typedef struct nano_buf_s {
  unsigned char *buf;
  size_t len;
  size_t cur;
} nano_buf;

typedef void (*sakura_sfunc)(nano_buf *, SEXP, SEXP);
typedef SEXP (*sakura_ufunc)(unsigned char *, size_t, SEXP);

#endif
