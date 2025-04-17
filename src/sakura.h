// sakura - header file --------------------------------------------------------

#ifndef SAKURA_H
#define SAKURA_H

#include <stdint.h>
#include <inttypes.h>
#ifndef R_NO_REMAP
#define R_NO_REMAP
#endif
#ifndef STRICT_R_HEADERS
#define STRICT_R_HEADERS
#endif
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Visibility.h>

typedef struct nano_buf_s {
  unsigned char *buf;
  size_t len;
  size_t cur;
} nano_buf;

// bundle of objects passed to hook function
// hook: serialization hook function passed from R
// output stream: output stream for serialization
typedef struct SakuraSerializeBundle {
  R_outpstream_t stream; // pointer to R_outpstream_st
  const char * klass_name;
  SEXP hook_func;
} SakuraSerializeBundle;

typedef struct SakuraUnserializeBundle {
  R_inpstream_t stream; // pointer to R_inpstream_st
  SEXP hook_func;
} SakuraUnserializeBundle;

#define SAKURA_INIT_BUFSIZE 4096
#define SAKURA_SERIAL_VER 3
#define SAKURA_SERIAL_THR 134217728

void sakura_serialize(nano_buf *, SEXP, SEXP, SEXP);
SEXP sakura_unserialize(unsigned char *, size_t, SEXP);

SEXP sakura_r_serialize(SEXP, SEXP, SEXP);
SEXP sakura_r_unserialize(SEXP, SEXP);

#endif
