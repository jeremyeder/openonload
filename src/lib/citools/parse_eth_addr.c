/*
** Copyright 2005-2012  Solarflare Communications Inc.
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
  
/*! \cidoxg_lib_citools */

#include "citools_internal.h"


int ci_parse_eth_addr(void* eth_mac_addr, const char* str, char sep)
{
  unsigned b1, b2, b3, b4, b5, b6;
  unsigned char* p;

  ci_assert(eth_mac_addr);
  ci_assert(str);

  if( strlen(str) < 17 )  return -1;

  if( sep ) {
    char fmt[] = "%02x:%02x:%02x:%02x:%02x:%02x";
    fmt[4] = fmt[9] = fmt[14] = fmt[19] = fmt[24] = sep;
    if( ci_sscanf(str, fmt, &b1, &b2, &b3, &b4, &b5, &b6) != 6 )
      return -1;
  }
  else {
    if( ci_sscanf(str, "%02x%c%02x%c%02x%c%02x%c%02x%c%02x",
	       &b1, &sep, &b2, &sep, &b3, &sep,
	       &b4, &sep, &b5, &sep, &b6) != 11 )
      return -1;
  }

  p = (unsigned char*) eth_mac_addr;
  p[0] = (unsigned char) b1;
  p[1] = (unsigned char) b2;
  p[2] = (unsigned char) b3;
  p[3] = (unsigned char) b4;
  p[4] = (unsigned char) b5;
  p[5] = (unsigned char) b6;

  return 0;
}

/*! \cidoxg_end */
