// sakura - package level registrations ----------------------------------------

#include "sakura.h"

static const R_CallMethodDef callMethods[] = {
  {"sakura_r_serialize", (DL_FUNC) &sakura_r_serialize, 3},
  {"sakura_r_unserialize", (DL_FUNC) &sakura_r_unserialize, 2},
  {NULL, NULL, 0}
};

void attribute_visible R_init_sakura(DllInfo* dll) {
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
  R_RegisterCCallable("sakura", "sakura_serialize", (DL_FUNC) &sakura_serialize);
  R_RegisterCCallable("sakura", "sakura_unserialize", (DL_FUNC) &sakura_unserialize);
}
