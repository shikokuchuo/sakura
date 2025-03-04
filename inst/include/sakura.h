// Copyright (C) 2025 Hibiki AI Limited <info@hibiki-ai.com>
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

// sakura - public header file -------------------------------------------------

#ifndef SAKURA_H
#define SAKURA_H

#include <R.h>
#include <Rinternals.h>

typedef struct nano_buf_s {
  unsigned char *buf;
  size_t len;
  size_t cur;
} nano_buf;

typedef void (*sakura_sfunc)(nano_buf *, SEXP, SEXP);
typedef SEXP (*sakura_ufunc)(unsigned char *, size_t, SEXP);

#endif
