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

// sakura - header file --------------------------------------------------------

#ifndef SAKURA_H
#define SAKURA_H

#include <stdint.h>
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

#define SAKURA_INIT_BUFSIZE 4096
#define SAKURA_SERIAL_VER 3
#define SAKURA_SERIAL_THR 134217728
#define SAKURA_LD_STRLEN 21

void sakura_serialize(nano_buf *, SEXP, SEXP);
SEXP sakura_unserialize(unsigned char *, size_t, SEXP);

SEXP sakura_r_serialize(SEXP, SEXP);
SEXP sakura_r_unserialize(SEXP, SEXP);

#endif
