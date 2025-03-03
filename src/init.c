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

// sakura - package level registrations ----------------------------------------

#include "sakura.h"

static const R_CallMethodDef callMethods[] = {
  {"sakura_r_serialize", (DL_FUNC) &sakura_r_serialize, 2},
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
