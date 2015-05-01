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
** \author  
**  \brief  
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_lib_transport_unix */

#include <internal.h>


static int citp_timestamp_compare(const ci_uint32 a_sec, const ci_uint32 a_nsec,
                                  const ci_uint32 b_sec, const ci_uint32 b_nsec)
{
  if( a_sec < b_sec ) {
    return -1;
  }
  else if( a_sec == b_sec ) {
    if( a_nsec < b_nsec )
      return -1;
    else if( a_nsec == b_nsec )
      return 0;
    else
      return 1;
  }
  else {
    return 1;
  }
}


int citp_timespec_compare(const struct timespec* a, const struct timespec* b)
{
  return citp_timestamp_compare(a->tv_sec, a->tv_nsec, b->tv_sec, b->tv_nsec);
}


int citp_oo_timespec_compare(const struct oo_timespec* a,
                             const struct timespec* b)
{
  return citp_timestamp_compare(a->tv_sec, a->tv_nsec, b->tv_sec, b->tv_nsec);
}


void citp_oo_get_cpu_khz(ci_uint32* cpu_khz)
{
  ef_driver_handle fd;

  /* set up a constant value for the case everything goes wrong */
  *cpu_khz = 1000;

  if( ef_onload_driver_open(&fd, OO_STACK_DEV, 1) != 0 ) {
    fprintf(stderr, "%s: Failed to open /dev/onload\n", __FUNCTION__);
    ci_get_cpu_khz(cpu_khz);
    return;
  }
  if( ci_sys_ioctl(fd, OO_IOC_GET_CPU_KHZ, cpu_khz) != 0 ) {
    Log_E(log("%s: Failed to query cpu_khz", __FUNCTION__));
    ci_get_cpu_khz(cpu_khz);
  }
  ef_onload_driver_close(fd);
}

/*! \cidoxg_end */
