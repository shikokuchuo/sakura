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

typedef struct sakura_serial_bundle_s {
  R_outpstream_t stream;
  SEXP klass;
  SEXP hook_func;
} sakura_serial_bundle;

typedef struct sakura_unserial_bundle_s {
  R_inpstream_t stream;
  SEXP hook_func;
} sakura_unserial_bundle;

#define SAKURA_INIT_BUFSIZE 4096
#define SAKURA_SERIAL_VER 3
#define SAKURA_SERIAL_THR 134217728

void sakura_serialize(nano_buf *, SEXP, SEXP);
SEXP sakura_unserialize(unsigned char *, size_t, SEXP);

SEXP sakura_r_serialize(SEXP, SEXP);
SEXP sakura_r_unserialize(SEXP, SEXP);

#endif
