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
** \author  djr
**  \brief  
**   \date  2003/06/03
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/
  
/*! \cidoxg_lib_transport_ip */
  
#include "ip_internal.h"
#include <onload/ul/per_thread.h>


/* By default, log anything unexpected that happens. */
unsigned ci_tp_log = CI_TP_LOG_E | CI_TP_LOG_U | CI_TP_LOG_DU;
unsigned ci_tp_max_dump = 80;


int ci_tp_init(citp_init_thread_callback cb)
{
  const char* s;

#ifndef NDEBUG
  static int done = 0;
  ci_assert(!done);
  done = 1;
#endif

  /*! ?? \TODO setup config options etc. */
  if( (s = getenv("TP_LOG")) )  sscanf(s, "%x", &ci_tp_log);
  LOG_S(log("TP_LOG = %x", ci_tp_log));

  init_thread_callback = cb;
  oo_per_thread_init();

  return 0;
}

/*! \cidoxg_end */
