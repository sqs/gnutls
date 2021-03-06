/* GnuTLS --- Guile bindings for GnuTLS.
   Copyright (C) 2007, 2010 Free Software Foundation, Inc.

   GnuTLS is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   GnuTLS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GnuTLS; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA  */

#ifndef GUILE_GNUTLS_ERRORS_H
#define GUILE_GNUTLS_ERRORS_H

#include <libguile.h>

#include "utils.h"

SCM_API void
scm_gnutls_error (int, const char *)
  NO_RETURN;
     SCM_API void scm_init_gnutls_error (void);

#endif

/* arch-tag: e7a92e44-b399-4c85-99d4-2dd3564600f7
 */
