// Copyright (C) 2024-2025 Hibiki AI Limited <info@hibiki-ai.com>
//
// This file is part of sakura.
//
// sakura is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// sakura is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// sakura. If not, see <https://www.gnu.org/licenses/>.

// sakura - Core Functions -----------------------------------------------------

#include "sakura.h"

// internal functions ----------------------------------------------------------

static SEXP nano_eval_res;

static void nano_eval_safe (void *call) {
  nano_eval_res = Rf_eval((SEXP) call, R_GlobalEnv);
}

static void nano_write_bytes(R_outpstream_t stream, void *src, int len) {

  nano_buf *buf = (nano_buf *) stream->data;

  size_t req = buf->cur + (size_t) len;
  if (req > buf->len) {
    if (req > R_XLEN_T_MAX) {
      if (buf->len) R_Free(buf->buf);
      Rf_error("serialization exceeds max length of raw vector");
    }
    do {
      buf->len += buf->len > SAKURA_SERIAL_THR ? SAKURA_SERIAL_THR : buf->len;
    } while (buf->len < req);
    buf->buf = R_Realloc(buf->buf, buf->len, unsigned char);
  }

  memcpy(buf->buf + buf->cur, src, len);
  buf->cur += len;

}

static void nano_read_bytes(R_inpstream_t stream, void *dst, int len) {

  nano_buf *buf = (nano_buf *) stream->data;
  if (buf->cur + len > buf->len) Rf_error("unserialization error");
  memcpy(dst, buf->buf + buf->cur, len);
  buf->cur += len;

}

static SEXP nano_inHook(SEXP x, SEXP hook) {

  if (!Rf_inherits(x, CHAR(STRING_ELT(CAR(hook), 0))))
    return R_NilValue;

  SEXP newlist, list, newnames, names, out;
  R_xlen_t xlen;

  list = TAG(hook);
  xlen = Rf_xlength(list);
  PROTECT(names = Rf_getAttrib(list, R_NamesSymbol));

  char idx[SAKURA_LD_STRLEN];
  snprintf(idx, SAKURA_LD_STRLEN, "%ld", (long) (xlen + 1));
  PROTECT(out = Rf_mkChar(idx));

  PROTECT(newlist = Rf_allocVector(VECSXP, xlen + 1));
  PROTECT(newnames = Rf_allocVector(STRSXP, xlen + 1));
  for (R_xlen_t i = 0; i < xlen; i++) {
    SET_VECTOR_ELT(newlist, i, VECTOR_ELT(list, i));
    SET_STRING_ELT(newnames, i, STRING_ELT(names, i));
  }
  SET_VECTOR_ELT(newlist, xlen, x);
  SET_STRING_ELT(newnames, xlen, out);

  Rf_namesgets(newlist, newnames);
  SET_TAG(hook, newlist);

  UNPROTECT(4);
  return Rf_ScalarString(out);

}

static SEXP nano_outHook(SEXP x, SEXP fun) {

  const long i = atol(CHAR(STRING_ELT(x, 0))) - 1;
  return VECTOR_ELT(fun, i);

}

// C API functions -------------------------------------------------------------

void sakura_serialize(nano_buf *buf, SEXP object, SEXP hook) {

  buf->buf = R_Calloc(SAKURA_INIT_BUFSIZE, unsigned char);
  buf->len = SAKURA_INIT_BUFSIZE;
  buf->cur = 0;

  const int reg = hook != R_NilValue;
  int vec;

  if (reg) {
    vec = reg ? INTEGER(CADDDR(hook))[0] : 0;
    buf->buf[0] = 0x7;
    buf->buf[1] = (uint8_t) vec;
    buf->cur += 12;
  }

  struct R_outpstream_st output_stream;

  R_InitOutPStream(
    &output_stream,
    (R_pstream_data_t) buf,
#ifdef WORDS_BIGENDIAN
    R_pstream_xdr_format,
#else
    R_pstream_binary_format,
#endif
    SAKURA_SERIAL_VER,
    NULL,
    nano_write_bytes,
    reg ? nano_inHook : NULL,
    hook
  );

  R_Serialize(object, &output_stream);

  if (reg && TAG(hook) != R_NilValue) {
    const uint64_t cursor = (uint64_t) buf->cur;
    memcpy(buf->buf + 4, &cursor, sizeof(uint64_t));
    SEXP call;

    if (vec) {

      PROTECT(call = Rf_lcons(CADR(hook), Rf_cons(TAG(hook), R_NilValue)));
      if (R_ToplevelExec(nano_eval_safe, call) &&
          TYPEOF(nano_eval_res) == RAWSXP) {
        R_xlen_t xlen = XLENGTH(nano_eval_res);
        if (buf->cur + xlen > buf->len) {
          buf->len = buf->cur + xlen;
          buf->buf = R_Realloc(buf->buf, buf->len, unsigned char);
        }
        memcpy(buf->buf + buf->cur, DATAPTR_RO(nano_eval_res), xlen);
        buf->cur += xlen;
      }
      UNPROTECT(1);

    } else {

      SEXP refList = TAG(hook);
      SEXP func = CADR(hook);
      R_xlen_t llen = Rf_xlength(refList);
      if (buf->cur + sizeof(R_xlen_t) > buf->len) {
        buf->len = buf->cur + SAKURA_INIT_BUFSIZE;
        buf->buf = R_Realloc(buf->buf, buf->len, unsigned char);
      }
      memcpy(buf->buf + buf->cur, &llen, sizeof(R_xlen_t));
      buf->cur += sizeof(R_xlen_t);

      for (R_xlen_t i = 0; i < llen; i++) {
        PROTECT(call = Rf_lcons(func, Rf_cons(VECTOR_ELT(refList, i), R_NilValue)));
        if (R_ToplevelExec(nano_eval_safe, call) &&
            TYPEOF(nano_eval_res) == RAWSXP) {
          R_xlen_t xlen = XLENGTH(nano_eval_res);
          if (buf->cur + xlen + sizeof(R_xlen_t) > buf->len) {
            buf->len = buf->cur + xlen + sizeof(R_xlen_t);
            buf->buf = R_Realloc(buf->buf, buf->len, unsigned char);
          }
          memcpy(buf->buf + buf->cur, &xlen, sizeof(R_xlen_t));
          buf->cur += sizeof(R_xlen_t);
          memcpy(buf->buf + buf->cur, DATAPTR_RO(nano_eval_res), xlen);
          buf->cur += xlen;
        }
        UNPROTECT(1);
      }

    }

    SET_TAG(hook, R_NilValue);

  }

}

SEXP sakura_unserialize(unsigned char *buf, size_t sz, SEXP hook) {

  uint64_t offset;
  size_t cur;
  SEXP reflist;

  if (sz > 12) {
    switch (buf[0]) {
    case 0x41:
    case 0x42:
    case 0x58:
      offset = 0;
      cur = 0;
      goto resume;
    case 0x7: ;
      memcpy(&offset, buf + 4, sizeof(uint64_t));
      if (offset) {
        SEXP raw, call;
        if (buf[1]) {
          PROTECT(raw = Rf_allocVector(RAWSXP, sz - offset));
          memcpy(RAW(raw), buf + offset, sz - offset);
          PROTECT(call = Rf_lcons(CADDR(hook), Rf_cons(raw, R_NilValue)));
          reflist = Rf_eval(call, R_GlobalEnv);
          UNPROTECT(2);
        } else {
          R_xlen_t llen, xlen;
          memcpy(&llen, buf + offset, sizeof(R_xlen_t));
          cur = offset + sizeof(R_xlen_t);
          PROTECT(reflist = Rf_allocVector(VECSXP, llen));
          SEXP out;
          SEXP func = CADDR(hook);
          for (R_xlen_t i = 0; i < llen; i++) {
            memcpy(&xlen, buf + cur, sizeof(R_xlen_t));
            cur += sizeof(R_xlen_t);
            PROTECT(raw = Rf_allocVector(RAWSXP, xlen));
            memcpy(RAW(raw), buf + cur, xlen);
            cur += xlen;
            PROTECT(call = Rf_lcons(func, Rf_cons(raw, R_NilValue)));
            out = Rf_eval(call, R_GlobalEnv);
            SET_VECTOR_ELT(reflist, i, out);
            UNPROTECT(2);
          }
          UNPROTECT(1);
        }
        SET_TAG(hook, reflist);
      }
      cur = 12;
      goto resume;
    }
  }

  Rf_error("data could not be unserialized");

  resume: ;

  SEXP out;
  nano_buf nbuf;
  struct R_inpstream_st input_stream;

  nbuf.buf = buf;
  nbuf.len = sz;
  nbuf.cur = cur;

  R_InitInPStream(
    &input_stream,
    (R_pstream_data_t) &nbuf,
    R_pstream_any_format,
    NULL,
    nano_read_bytes,
    offset ? nano_outHook : NULL,
    offset ? reflist : R_NilValue
  );

  out = R_Unserialize(&input_stream);

  if (offset)
    SET_TAG(hook, R_NilValue);

  return out;

}

// R API functions -------------------------------------------------------------

SEXP sakura_r_serialize(SEXP object, SEXP hook) {

  nano_buf buf;
  sakura_serialize(&buf, object, hook);

  SEXP out = Rf_allocVector(RAWSXP, buf.cur);
  memcpy(RAW(out), buf.buf, buf.cur);
  R_Free(buf.buf);

  return out;

}

SEXP sakura_r_unserialize(SEXP object, SEXP hook) {

  return sakura_unserialize(RAW(object), XLENGTH(object), hook);

}
