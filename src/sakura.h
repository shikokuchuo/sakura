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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct nano_buf_s {
  unsigned char *buf;
  size_t len;
  size_t cur;
} nano_buf;

// --------------------------------------------------------
// custom hash map of strings (const char *) to SEXP
// It has a max hash size of `MAX_SEXPMAP_SIZE` 32 entries. 
// Additional entries would not be hashed but stored as is (CharacterVector and list of functions)
// We use this approach so we don't have to dynamically manage memory for the hash table.
// The hashmap should cover all use cases for realistic usage, but can be updated in the future. 

#define MAX_SEXPMAP_SIZE 32
#define SEXPMAP_MASK (MAX_SEXPMAP_SIZE - 1u)

typedef struct {
    const char *key;
    SEXP        val;
} SEXPEntry;

typedef struct {
    size_t     len;
    SEXPEntry  slot[MAX_SEXPMAP_SIZE];
    SEXP       all_names;
    SEXP       all_values;
} SEXPMap;

typedef struct {
    SEXP result;
    const char * klass_name_used;
} sexpmap_inherits_result;

// djb2 hash
static uint32_t sexpmap_hash(const char *s) {
  uint32_t h = 5381u;
  for (unsigned char c; (c = (unsigned char)*s++); )
      h = (h << 5) + h + c;
  return h;
}

static SEXPMap sexpmap_create(SEXP func_names, SEXP functions) {
  if (TYPEOF(func_names) != STRSXP)
      Rf_error("`func_names` must be a character vector");
  R_xlen_t n = Rf_xlength(func_names);
  if (Rf_xlength(functions) != n)
  Rf_error("`func_names` and `functions` must have the same length");

  SEXPMap m = { 0 };
  m.len = n;
  m.all_names  = func_names;
  m.all_values = functions;

  /* insert up to MAX_SEXPMAP_SIZE entries into the hash table */
  R_xlen_t limit = n < MAX_SEXPMAP_SIZE ? n : MAX_SEXPMAP_SIZE;
  for (R_xlen_t i = 0; i < limit; ++i) {
      const char * key = CHAR(STRING_ELT(func_names, i));
      SEXP         val = VECTOR_ELT(functions, i);
      uint32_t     idx = sexpmap_hash(key) & SEXPMAP_MASK;

      for (size_t probe = 0; probe < MAX_SEXPMAP_SIZE; ++probe) {
          SEXPEntry *e = &m.slot[idx];
          if (e->key == NULL) {
              e->key = key;
              e->val = val;
              break;
          }
          if (strcmp(e->key, key) == 0) { /* duplicate key, should probably check before input */
            Rf_error("duplicate serialization config class name found: %s", key);
          }
          idx = (idx + 1u) & SEXPMAP_MASK;
      }
  }
  return m;
}

static SEXP sexpmap_get(const SEXPMap *m, const char *key) {
  uint32_t idx  = sexpmap_hash(key) & SEXPMAP_MASK;

  for (size_t probe = 0; probe < MAX_SEXPMAP_SIZE; ++probe) {
      const SEXPEntry *e = &m->slot[idx];
      if (e->key == NULL) 
          return R_NilValue; // not found in map, NULL also implies len < MAX_SEXPMAP_SIZE
      if (strcmp(e->key, key) == 0) 
          return e->val;
      idx = (idx + 1u) & SEXPMAP_MASK;
  }

  R_xlen_t n = Rf_xlength(m->all_names);
  for (R_xlen_t i = MAX_SEXPMAP_SIZE; i < n; ++i) {
      const char *k2 = CHAR(STRING_ELT(m->all_names, i));
      if (strcmp(k2, key) == 0)
          return VECTOR_ELT(m->all_values, i);
  }
  return R_NilValue;
}

// keys is a character vector and may be more than one
// this is equivalent to Rf_inherits
static sexpmap_inherits_result sexpmap_inherits(const SEXPMap *m, SEXP klass) {
  R_xlen_t n = Rf_xlength(klass);
  sexpmap_inherits_result out = {0};
  out.result = R_NilValue;
  for (R_xlen_t i = 0; i < n; ++i) {
    const char * key = CHAR(STRING_ELT(klass, i));
    SEXP result = sexpmap_get(m, key);
    if (result != R_NilValue) {
      out.result = result;
      out.klass_name_used = key;
      return out;
    }
  }
  return out;
}

// Rf_inherits for reference
// INLINE_FUN Rboolean inherits(SEXP s, const char *name)
// {
//     SEXP klass;
//     int i, nclass;
//     if (OBJECT(s)) {
// 	klass = getAttrib(s, R_ClassSymbol);
// 	nclass = length(klass);
// 	for (i = 0; i < nclass; i++) {
// 	    if (!strcmp(CHAR(STRING_ELT(klass, i)), name))
// 		return TRUE;
// 	}
//     }
//     return FALSE;
// }

// --------------------------------------------------------
// Bundle definitions

typedef struct sakura_serial_bundle_s {
  R_outpstream_t stream;
  SEXPMap hook_func_map;
} sakura_serial_bundle;

typedef struct sakura_unserial_bundle_s {
  R_inpstream_t stream;
  SEXPMap hook_func_map;
} sakura_unserial_bundle;

#define SAKURA_INIT_BUFSIZE 4096
#define SAKURA_SERIAL_VER 3
#define SAKURA_SERIAL_THR 134217728
#define SAKURA_CHUNK_SIZE INT_MAX // must be <= INT_MAX

extern SEXP sakura_bundle;

void sakura_serialize(nano_buf *, SEXP, SEXP);
SEXP sakura_unserialize(unsigned char *, size_t, SEXP);
void sakura_serialize_init(SEXP, R_outpstream_t, R_pstream_data_t, SEXP, SEXP, void (*)(R_outpstream_t, void *, int));
void sakura_unserialize_init(SEXP, R_inpstream_t, R_pstream_data_t, SEXP, void (*)(R_inpstream_t, void *, int));

SEXP sakura_r_serialize(SEXP, SEXP);
SEXP sakura_r_unserialize(SEXP, SEXP);


#endif
