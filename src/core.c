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

static SEXP nano_serializeHook(SEXP x, SEXP bundle_xptr) {
  // unbundle relevant objects, klass_name, hook_func and OutBytes
  SakuraSerializeBundle * bundle = (SakuraSerializeBundle *) R_ExternalPtrAddr(bundle_xptr);
  R_outpstream_t stream = bundle->stream;
  const char * klass_name = bundle->klass_name;
  SEXP hook_func = bundle->hook_func;
  void (*OutBytes)(R_outpstream_t, void *, int) = stream->OutBytes;

  if (!Rf_inherits(x, klass_name)) {
    return R_NilValue;
  }

  // serialize the object by calling hook_func, which returns a raw vector
  SEXP call = PROTECT(Rf_lcons(hook_func, Rf_cons(x, R_NilValue)));
  if (R_ToplevelExec(nano_eval_safe, call) && TYPEOF(nano_eval_res) == RAWSXP) {
    // write out PERSISTXP manually
    uint64_t size = Rf_xlength(nano_eval_res);
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
    R_xlen_t xlen = XLENGTH(nano_eval_res);
    OutBytes(stream, RAW(nano_eval_res), xlen);

    UNPROTECT(1);
    return Rf_mkString(size_string); // this will write the PERSISTXP again
  } else { // else something went really wrong
    UNPROTECT(1);
    return R_NilValue;
  }
}

static SEXP nano_unserializeHook(SEXP x, SEXP bundle_xptr) {
  // unbundle relevant objects, hook_func and InBytes
  SakuraUnserializeBundle * bundle = (SakuraUnserializeBundle *) R_ExternalPtrAddr(bundle_xptr);
  R_inpstream_t stream = bundle->stream;
  SEXP hook_func = bundle->hook_func;
  void (*InBytes)(R_inpstream_t, void *, int) = stream->InBytes;

  const char *size_string = CHAR(STRING_ELT(x, 0));
  if (strlen(size_string) != 20)
    Rf_error("Invalid string length for uint64_t conversion");
  uint64_t size = strtoul(size_string, NULL, 10);

  // read in the binary serialization blob
  SEXP raw = PROTECT(Rf_allocVector(RAWSXP, size));
  InBytes(stream, RAW(raw), size);

  // read in 40 additional bytes and discard them
  char buf[40];
  InBytes(stream, buf, 40);

  // call hook_func to unserialize the object
  SEXP call = PROTECT(Rf_lcons(hook_func, Rf_cons(raw, R_NilValue)));
  SEXP out = Rf_eval(call, R_GlobalEnv);
  UNPROTECT(2);
  return out;
}

// C API functions -------------------------------------------------------------


void sakura_serialize_init(
    SEXP bundle_xptr, // XPtr to SakuraSerializeBundle
    R_outpstream_t stream, // pointer to R_outpstream_st
    R_pstream_data_t data, // void *, arbitrary data passed to OutBytes
    const char * klass_name, // class name to serialize
    SEXP hook_func, // hook function to serialize
    void (*outbytes)(R_outpstream_t, void *, int))
{
  SakuraSerializeBundle * bundle = (SakuraSerializeBundle *) R_ExternalPtrAddr(bundle_xptr);

  bundle->klass_name = klass_name;
  bundle->hook_func = hook_func;
  R_InitOutPStream(
    stream,
    data,
    R_pstream_binary_format,
    SAKURA_SERIAL_VER, // 3
    NULL, // OutChar
    outbytes, // OutBytes
    nano_serializeHook,
    bundle_xptr
  );
  bundle->stream = stream;
}


void sakura_unserialize_init(
    SEXP bundle_xptr, // XPtr to SakuraUnserializeBundle
    R_inpstream_t stream, // pointer to R_inpstream_st
    R_pstream_data_t data, // void *, arbitrary data passed to InBytes
    SEXP hook_func, // hook function to unserialize
    void (*inbytes)(R_inpstream_t, void *, int))
{
  SakuraUnserializeBundle * bundle = (SakuraUnserializeBundle *) R_ExternalPtrAddr(bundle_xptr);

  bundle->hook_func = hook_func;
  R_InitInPStream(
    stream,
    data,
    R_pstream_binary_format,
    NULL, // InChar
    inbytes, // InBytes
    nano_unserializeHook,
    bundle_xptr
  );
  bundle->stream = stream;
}


void sakura_serialize(nano_buf *buf, SEXP object, SEXP klass_name, SEXP hook_func) {
  buf->buf = R_Calloc(SAKURA_INIT_BUFSIZE, unsigned char);
  buf->len = SAKURA_INIT_BUFSIZE;
  buf->cur = 0;

  SakuraSerializeBundle b;
  SEXP bundle;
  PROTECT(bundle = R_MakeExternalPtr(&b, R_NilValue, R_NilValue));
  struct R_outpstream_st output_stream;

  sakura_serialize_init(
    bundle,
    &output_stream,
    (R_pstream_data_t) buf,
    CHAR(STRING_ELT(klass_name, 0)),
    hook_func,
    nano_write_bytes);

  R_Serialize(object, &output_stream);
  UNPROTECT(1);
}


SEXP sakura_unserialize(unsigned char *buf, size_t sz, SEXP hook) {
  nano_buf nbuf;
  nbuf.buf = buf;
  nbuf.len = sz;
  nbuf.cur = 0;
  struct R_inpstream_st input_stream;

  SakuraSerializeBundle b;
  SEXP bundle, out;
  PROTECT(bundle = R_MakeExternalPtr(&b, R_NilValue, R_NilValue));

  sakura_unserialize_init(
    bundle,
    &input_stream,
    (R_pstream_data_t) &nbuf,
    hook,
    nano_read_bytes);

  out = R_Unserialize(&input_stream);
  UNPROTECT(1);
  return out;
}

// R API functions -------------------------------------------------------------

SEXP sakura_r_serialize(SEXP object, SEXP klass_name, SEXP hook_func) {

  nano_buf buf;
  sakura_serialize(&buf, object, klass_name, hook_func);

  SEXP out = Rf_allocVector(RAWSXP, buf.cur);
  memcpy(RAW(out), buf.buf, buf.cur);
  R_Free(buf.buf);

  return out;

}

SEXP sakura_r_unserialize(SEXP object, SEXP hook_func) {

  return sakura_unserialize(RAW(object), XLENGTH(object), hook_func);

}
