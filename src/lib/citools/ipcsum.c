/*
** Copyright 2005-2013  Solarflare Communications Inc.
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
**  \brief  Compute Internet checksums.
**   \date  2003/01/05
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/
 
/*! \cidoxg_lib_citools */
 
#include "citools_internal.h"
#include <ci/net/ipv4.h>

/* 0xffff is an impossible checksum for TCP and IP (special case for UDP)
** This is because you would need the partial checksum when folded to be
** 0 (so it inverts to ffff). The checksum is additive so you can only
** add to the next multiple of 0x10000 and that will always get folded
** back again
*/

unsigned ci_ip_checksum(const ci_ip4_hdr* ip)
{
  const ci_uint16* p = (const ci_uint16*) ip;
  unsigned csum;
  int bytes;

  csum  = p[0];
  csum += p[1];
  csum += p[2];
  csum += p[3];
  csum += p[4];
  /* omit ip_check_be16 */
  csum += p[6];
  csum += p[7];
  csum += p[8];
  csum += p[9];

  bytes = CI_IP4_IHL(ip);
  if(CI_UNLIKELY( bytes > 20 )) {
    p += 10;
    bytes -= 20;
    do {
      csum += *p++;
      bytes -= 2;
    } while( bytes );
  }

  return ci_ip_hdr_csum_finish(csum);
}

/*! \cidoxg_end */
