/*
** Copyright 2005-2014  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of version 2 of the GNU General Public License as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/**************************************************************************\
*//*! \file
** <L5_PRIVATE L5_SOURCE>
** \author  
**  \brief  
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/
  
/*! \cidoxg_lib_transport_unix */
 
#include <internal.h>


#define LPF      "citp_epinfo_"

void citp_epinfo_assert_valid(citp_epinfo* epinfo)
{
  ci_assert(epinfo);
  CITP_PROTOCOL_IMPL_ASSERT_VALID(epinfo->protocol);
}


void citp_epinfo_init(citp_epinfo* epinfo, citp_protocol_impl* protocol)
{
  ci_assert(epinfo);
  ci_assert(protocol);

  epinfo->protocol = protocol;

  /* Start at zero.  It will be increased whenever an endpoint is inserted
  ** into the fdtable.
  */
  oo_atomic_set(&epinfo->ref_count, 0);
}


/* 2004/08/16 stg: added [fdt_locked] to allow the fd table to be
 * locked before this function.  [fdt_locked] = 0 for legacy operation
 */
void __citp_epinfo_ref_count_zero(citp_epinfo* epinfo,citp_fdinfo* last_fdinfo, 
				  int fdt_locked)
{
  Log_V(log(LPF "ref_count_zero(%p, %d)", epinfo, last_fdinfo->fd));

  ci_assert(epinfo);
  ci_assert(oo_atomic_read(&epinfo->ref_count) == 0);
  ci_assert(epinfo->protocol);
  ci_assert(last_fdinfo);
  ci_assert(last_fdinfo->ep == epinfo);

  epinfo->protocol->ops.dtor(epinfo, last_fdinfo, fdt_locked);
}


int citp_ep_ioctl(citp_fdinfo* fdinfo, unsigned long request, long arg)
{
  CITP_FDINFO_ASSERT_VALID(fdinfo);

  Log_V(log(LPF "ioctl(%d, %lu, %ld)", fdinfo->fd, request, arg));

  /*! \TODO see /usr/include/bits/ioctls.h for lots of socketey ones */

  ci_fail(("?? not yet implemented"));
  errno = ENOTSUP;
  return -1;
}


/*! \cidoxg_end */

