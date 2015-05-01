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
 
/*! \cidoxg_lib_citools */
 
#include "citools_internal.h"
#if defined(__FreeBSD__) || defined(__MACH__)
#include <sys/sysctl.h>
#endif

unsigned ci_cpu_khz;



# if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)

ci_inline int try_get_hz(const char* line, unsigned* cpu_khz_out)
{
  float f;

  if( sscanf(line, "cpu MHz : %f", &f) != 1 )  return 0;

  *cpu_khz_out = (unsigned) (f * 1000.0);
  return 1;
}

/* The following routine has been obtained sfnettest code */
static int ci_measure_cpu_khz(unsigned* cpu_khz)
{
  int interval_usec = 100000;
  struct timeval tv_s, tv_e;
  uint64_t tsc_s, tsc_e, tsc_e2;
  uint64_t tsc_gtod, min_tsc_gtod, usec = 0;
  int n, skew = 0;

  ci_frc64(&tsc_s);
  gettimeofday(&tv_s, NULL);
  ci_frc64(&tsc_e2);
  min_tsc_gtod = tsc_e2 - tsc_s;
  n = 0;
  do {
    ci_frc64(&tsc_s);
    gettimeofday(&tv_s, NULL);
    ci_frc64(&tsc_e2);
    tsc_gtod = tsc_e2 - tsc_s;
    if( tsc_gtod < min_tsc_gtod )
      min_tsc_gtod = tsc_gtod;
  } while( ++n < 20 || (tsc_gtod > min_tsc_gtod * 2 && n < 100) );

  do {
    ci_frc64(&tsc_e);
    gettimeofday(&tv_e, NULL);
    ci_frc64(&tsc_e2);
    if( tsc_e2 < tsc_e ) {
      skew = 1;
      break;
    }
    tsc_gtod = tsc_e2 - tsc_e;
    usec = (tv_e.tv_sec - tv_s.tv_sec) * (uint64_t) 1000000;
    usec += tv_e.tv_usec - tv_s.tv_usec;
  } while( usec < interval_usec || tsc_gtod > min_tsc_gtod * 2 );

  if( skew )
    return 0;
  *cpu_khz = (tsc_e - tsc_s) * 1000 / usec;
  return 1;
}


# elif defined(__PPC__)

/*
 * On PPC Suse 9 linux  /proc/cpuinfo gives cpu speed in the format ..
 *  .
 *  clock           : 1655.984000MHz
 *  .
 */

ci_inline int try_get_hz(const char* line, unsigned* cpu_khz_out)
{
  long l;

  if( sscanf(line, "timebase           : %lu", &l) != 1 )  
  	return 0;

  *cpu_khz_out = (unsigned) (l / 1000);
  return 1;
}


# else
#  error "ci: Dont know how to get cpu frequency."
# endif

int
ci_get_cpu_khz(unsigned* cpu_khz_out)
{
  FILE* f;
  char buf[80];

  if( ! ci_cpu_khz ) {
    /* On powerpc /proc/cpuinfo gives reliable information, hence no need to
     * measure.
     * On x86 cpuinfo readings might be ugely inaccurate when cpu_scaling
     * is enabled */
#ifndef __powerpc__
    if( ci_measure_cpu_khz(&ci_cpu_khz) )
      goto end;
    else
      ci_log("Warning measured cpu_khz not stable, querying /proc/cpuinfo");
#endif
    /* We only go get the khz if we need to.  Obviously it's sensible for
     * performance, but also we need to do this because we can't call fclose
     * once the system is fully initialized, since our overridden version of
     * fclose needs to get the fdtable-lock.  (Note: we would ideally just
     * ensure we always call the 'real' libc fclose from here, but since this
     * gets linked into the ciapp library, finding real libc is not so easy)
     * Therefore, it is important that this function get called early, to
     * ensure that we can't deadlock on ourselves by calling fclose when the
     * fdtable lock is held.
     */
    f = fopen("/proc/cpuinfo", "r");
    if( !f )  return -errno;

    while( 1 ) {
      if( !fgets(buf, sizeof(buf), f) )  {
        fclose (f);
        return -EIO;
      }
      if( try_get_hz(buf, &ci_cpu_khz) )  break;
    }

    fclose (f);
  }
#ifndef __powerpc__
end:
#endif
  if( cpu_khz_out )  *cpu_khz_out = ci_cpu_khz;
  return 0;
}



#if defined(__FreeBSD__) || defined(__MACH__)
int
ci_get_cpu_khz(unsigned* cpu_khz_out)
{
  size_t size;

  if( ! ci_cpu_khz ) {
#if defined(__FreeBSD__) 
    /*FreeBSD returns the frequency in MHz.*/
    int cpu_mhz;
    size = sizeof(cpu_mhz);
    if( sysctlbyname("dev.cpu.0.freq", &cpu_mhz, &size, NULL, 0) ) 
      return -errno;
    if( size != sizeof(cpu_mhz) )
      return -EIO;
    ci_cpu_khz = cpu_mhz * 1000;
#else
    /*OS X returns the frequency in Hertz.*/
    ci_uint64 cpu_hertz;
    size = sizeof(cpu_hertz);
    if( sysctlbyname("hw.cpufrequency", &cpu_hertz, &size, NULL, 0) )
          return -errno;
    if( size != sizeof(cpu_hertz) )
      return -EIO;
    ci_cpu_khz = (unsigned) cpu_hertz / 1000;
#endif

  }

  if( cpu_khz_out )
    *cpu_khz_out = ci_cpu_khz;

  return 0;
}

#endif	/*__FreeBSD__*/





/*! \cidoxg_end */
