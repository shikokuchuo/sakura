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

static SEXP nano_serialize_hook(SEXP x, SEXP bundle_xptr) {

  sakura_serial_bundle *bundle = (sakura_serial_bundle *) R_ExternalPtrAddr(bundle_xptr);
  R_outpstream_t stream = bundle->stream;
  SEXP klass = bundle->klass;
  SEXP hook_func = bundle->hook_func;
  int len = (int) XLENGTH(klass), i;
  void (*OutBytes)(R_outpstream_t, void *, int) = stream->OutBytes;

  for (i = 1; i <= len; i++) {
    if (Rf_inherits(x, CHAR(STRING_ELT(klass, i - 1))))
      goto resume;
  }

  return R_NilValue;

  resume:

  if (len == 1) i = 0;
  SEXP call;
  PROTECT(call = Rf_lcons(i ? VECTOR_ELT(hook_func, i - 1) : hook_func, Rf_cons(x, R_NilValue)));
  if (!R_ToplevelExec(nano_eval_safe, call) || TYPEOF(nano_eval_res) != RAWSXP) {
    // something went wrong
    UNPROTECT(1);
    return R_NilValue;
  }
  UNPROTECT(1);

  // write out PERSISTSXP manually
  uint64_t size = XLENGTH(nano_eval_res);
  char size_string[21];
  snprintf(size_string, sizeof(size_string), "%020" PRIu64, size);

  // should be const but OutBytes uses non-const pointer even though it should be const
  static int int_0 = 0;
  static int int_1 = 1;
  static int int_20 = 20;
  static int int_charsxp = CHARSXP;
  static int int_persistsxp = 247;

  // OutInteger(stream, PERSISTSXP);              // Total bytes
  OutBytes(stream, &int_persistsxp, sizeof(int)); // 4
  // OutStringVec(stream, t, ref_table);
  OutBytes(stream, &int_0, sizeof(int));          // 8
  OutBytes(stream, &int_1, sizeof(int));          // 12
  OutBytes(stream, &int_charsxp, sizeof(int));    // 16
  OutBytes(stream, &int_20, sizeof(int));         // 20
  OutBytes(stream, &size_string[0], 20);      // 40

  // write out binary serialization blob
  unsigned char *dest = RAW(nano_eval_res);
  while (size > SAKURA_CHUNK_SIZE) {
    OutBytes(stream, dest, SAKURA_CHUNK_SIZE);
    dest += SAKURA_CHUNK_SIZE;
    size -= SAKURA_CHUNK_SIZE;
  }
  OutBytes(stream, dest, (int) size);
  // write out index if hook_func is a vector
  OutBytes(stream, &i, sizeof(int));      // 4

  return R_BlankScalarString; // this will write the PERSISTSXP again

}

static SEXP nano_unserialize_hook(SEXP x, SEXP bundle_xptr) {

  sakura_unserial_bundle *bundle = (sakura_unserial_bundle *) R_ExternalPtrAddr(bundle_xptr);
  R_inpstream_t stream = bundle->stream;
  SEXP hook_func = bundle->hook_func;
  void (*InBytes)(R_inpstream_t, void *, int) = stream->InBytes;

  const char *size_string = CHAR(STRING_ELT(x, 0));
  uint64_t size = strtoul(size_string, NULL, 10);

  SEXP raw, call, out;
  PROTECT(raw = Rf_allocVector(RAWSXP, size));
  unsigned char *dest = RAW(raw);
  while (size > SAKURA_CHUNK_SIZE) {
    InBytes(stream, dest, SAKURA_CHUNK_SIZE);
    dest += SAKURA_CHUNK_SIZE;
    size -= SAKURA_CHUNK_SIZE;
  }
  InBytes(stream, dest, (int) size);

  int i;
  InBytes(stream, &i, 4);

  // read in 20 additional bytes and discard them
  char buf[20];
  InBytes(stream, buf, 20);

  PROTECT(call = Rf_lcons(i ? VECTOR_ELT(hook_func, i - 1) : hook_func, Rf_cons(raw, R_NilValue)));
  out = Rf_eval(call, R_GlobalEnv);

  UNPROTECT(2);
  return out;

}

// C API functions -------------------------------------------------------------

void sakura_serialize_init(SEXP bundle_xptr, R_outpstream_t stream, R_pstream_data_t data,
                           SEXP klass, SEXP hook_func, void (*outbytes)(R_outpstream_t, void *, int)) {

  sakura_serial_bundle *bundle = (sakura_serial_bundle *) R_ExternalPtrAddr(bundle_xptr);

  bundle->klass = klass;
  bundle->hook_func = hook_func;
  R_InitOutPStream(
    stream,
    data,
    R_pstream_binary_format,
    SAKURA_SERIAL_VER,
    NULL,
    outbytes,
    nano_serialize_hook,
    bundle_xptr
  );
  bundle->stream = stream;

}

void sakura_unserialize_init(SEXP bundle_xptr, R_inpstream_t stream, R_pstream_data_t data,
                             SEXP hook_func, void (*inbytes)(R_inpstream_t, void *, int)) {

  sakura_unserial_bundle *bundle = (sakura_unserial_bundle *) R_ExternalPtrAddr(bundle_xptr);

  bundle->hook_func = hook_func;
  R_InitInPStream(
    stream,
    data,
    R_pstream_any_format,
    NULL,
    inbytes,
    nano_unserialize_hook,
    bundle_xptr
  );
  bundle->stream = stream;

}

void sakura_serialize(nano_buf *buf, SEXP object, SEXP hook) {

  buf->buf = R_Calloc(SAKURA_INIT_BUFSIZE, unsigned char);
  buf->len = SAKURA_INIT_BUFSIZE;
  buf->cur = 0;

  struct R_outpstream_st output_stream;

  if (hook == R_NilValue) {

    R_InitOutPStream(
      &output_stream,
      (R_pstream_data_t) buf,
      R_pstream_binary_format,
      SAKURA_SERIAL_VER,
      NULL,
      nano_write_bytes,
      NULL,
      R_NilValue
    );

  } else {

    sakura_serial_bundle b;
    R_SetExternalPtrAddr(sakura_bundle, &b);

    sakura_serialize_init(
      sakura_bundle,
      &output_stream,
      (R_pstream_data_t) buf,
      CAR(hook),
      CADR(hook),
      nano_write_bytes
    );

  }

  R_Serialize(object, &output_stream);

}

SEXP sakura_unserialize(unsigned char *buf, size_t sz, SEXP hook) {

  nano_buf nbuf;
  nbuf.buf = buf;
  nbuf.len = sz;
  nbuf.cur = 0;

  struct R_inpstream_st input_stream;

  if (hook == R_NilValue) {

    R_InitInPStream(
      &input_stream,
      (R_pstream_data_t) &nbuf,
      R_pstream_any_format,
      NULL,
      nano_read_bytes,
      NULL,
      R_NilValue
    );

  } else {

    sakura_unserial_bundle b;
    R_SetExternalPtrAddr(sakura_bundle, &b);

    sakura_unserialize_init(
      sakura_bundle,
      &input_stream,
      (R_pstream_data_t) &nbuf,
      CADDR(hook),
      nano_read_bytes
    );

  }

  return R_Unserialize(&input_stream);

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
