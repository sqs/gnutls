/*
 * Copyright (C) 2004, 2005, 2008, 2009 Free Software Foundation
 *
 * Author: Simon Josefsson
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNUTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "utils.h"

/* See <http://bugs.gentoo.org/272388>. */

void
doit (void)
{
  int res;

  res = gnutls_global_init ();
  if (res != 0)
    fail ("gnutls_global_init\n");

  gnutls_global_deinit ();

  res = gnutls_global_init ();
  if (res != 0)
    fail ("gnutls_global_init2\n");

  gnutls_global_deinit ();

  success ("init-deinit round-trip success\n");
}