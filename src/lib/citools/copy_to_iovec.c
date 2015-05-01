/*
** Copyright 2005-2015  Solarflare Communications Inc.
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
 ** \author  djr
 **  \brief  Copy from ci_iovec_ptr to linear buffer.
 **   \date  2005/02/03
 **    \cop  (c) Level 5 Networks Limited.
 ** </L5_PRIVATE>
 *//*
 \**************************************************************************/

/*! \cidoxg_lib_citools */
#include "citools_internal.h"


int ci_copy_to_iovec(ci_iovec_ptr* dest, const void* src, int src_len)
{
  int total = 0, n;

  ci_assert(src || src_len == 0);
  ci_assert(src_len >= 0);

  while( 1 ) {
    n = CI_MIN((int) CI_IOVEC_LEN(&dest->io), src_len);
    memcpy(CI_IOVEC_BASE(&dest->io), src, n);
    src_len -= n;
    total += n;

    if( src_len == 0 ) {
      CI_IOVEC_BASE(&dest->io) = (char*)CI_IOVEC_BASE(&dest->io) + n;
      CI_IOVEC_LEN(&dest->io) -= n;
      return total;
    }

    /* Current segment of [dest] is exhausted. */
    ci_assert_equal(n, (int)CI_IOVEC_LEN(&dest->io));

    if( dest->iovlen == 0 ) {
      CI_IOVEC_LEN(&dest->io) = 0;
      return total;
    }

    src = (char*) src + n;
    --dest->iovlen;
    dest->io = *dest->iov++;
  }
}

/*! \cidoxg_end */
