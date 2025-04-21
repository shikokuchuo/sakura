// sakura - package level registrations ----------------------------------------

#include "sakura.h"

SEXP sakura_bundle;

static const R_CallMethodDef callMethods[] = {
  {"sakura_r_serialize", (DL_FUNC) &sakura_r_serialize, 2},
  {"sakura_r_unserialize", (DL_FUNC) &sakura_r_unserialize, 2},
  {NULL, NULL, 0}
};

static void register_callables(void) {
  R_RegisterCCallable("sakura", "sakura_serialize", (DL_FUNC) &sakura_serialize);
  R_RegisterCCallable("sakura", "sakura_unserialize", (DL_FUNC) &sakura_unserialize);
  R_RegisterCCallable("sakura", "sakura_serialize_init", (DL_FUNC) &sakura_serialize_init);
  R_RegisterCCallable("sakura", "sakura_unserialize_init", (DL_FUNC) &sakura_unserialize_init);
}

void attribute_visible R_init_sakura(DllInfo* dll) {
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
  R_PreserveObject(sakura_bundle = R_MakeExternalPtr(NULL, R_NilValue, R_NilValue));
  register_callables();
}

// # nocov start
void attribute_visible R_unload_sakura(DllInfo *info) {
  R_ReleaseObject(sakura_bundle);
}
// # nocov end
