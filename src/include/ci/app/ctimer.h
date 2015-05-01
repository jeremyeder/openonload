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
** <L5_PRIVATE L5_HEADER>
** \author  
**  \brief  
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_app */

#ifndef __CI_APP_CTIMER_H__
#define __CI_APP_CTIMER_H__


#if defined(CI_HAVE_FRC64)

# define ci_ctimer_frc    ci_frc64
# define ci_ctimer_frc32  ci_frc32
# define ci_ctimer_cast (ci_uint64 *)
# define CI_HAVE_CTIMER

#elif defined(CI_HAVE_FRC32)

# define ci_ctimer_frc    ci_frc32
# define ci_ctimer_frc32  ci_frc32
# define ci_ctimer_cast (ci_uint32 *)
# define CI_HAVE_CTIMER

#endif


#ifdef CI_HAVE_CTIMER


/*! Comment? */
typedef struct {
  ci_uint64  start;
  ci_uint64  stop;
} ci_ctimer;


/*! Comment? */
typedef struct {
  ci_int64  hz;
  ci_int64  overhead;
} ci_ctimer_inf;


/*! Comment? */
ci_inline void ci_ctimer_start(ci_ctimer* t)
{ ci_ctimer_frc( ci_ctimer_cast &t->start); }

/*! Comment? */
ci_inline void ci_ctimer_start_accurate(ci_ctimer* t) {
  ci_frc_flush();
  ci_ctimer_frc( ci_ctimer_cast &t->start);
}

/*! Comment? */
ci_inline void ci_ctimer_stop(ci_ctimer* t)
{ ci_ctimer_frc( ci_ctimer_cast &t->stop); }

/*! Comment? */
ci_inline void ci_ctimer_stop_accurate(ci_ctimer_inf* i, ci_ctimer* t) {
  ci_frc_flush();
  ci_ctimer_frc( ci_ctimer_cast &t->stop);
  t->stop -= i->overhead;
}

/*! Comment? */
ci_inline ci_int64 ci_ctimer_cycles(ci_ctimer* t)
{ return (ci_int64) (t->stop - t->start); }

/*! Comment? */
ci_inline ci_int64 ci_ctimer_usec(ci_ctimer_inf* i, ci_ctimer* t)
{ return (ci_int64) (t->stop - t->start) * 1000000 / i->hz; }

/*! Comment? */
ci_inline ci_int64 ci_ctimer_nsec(ci_ctimer_inf* i, ci_ctimer* t)
{ return (ci_int64) (t->stop - t->start) * 1000000 / (i->hz / 1000); }

/*! Comment? */
ci_inline ci_int64 ci_ctimer_Kbps(ci_ctimer_inf* i, ci_ctimer* t,
			      ci_int64 bytes)
{ return bytes * 8 * (i->hz / 1000) / (t->stop - t->start); }

/*! Comment? */
ci_inline ci_int64 ci_ctimer_Mbps(ci_ctimer_inf* i, ci_ctimer* t,
					ci_int64 bytes)
{ return bytes * 8 * (i->hz / 1000) / (t->stop - t->start) / 1000; }


/*! Comment? */
ci_inline void ci_ctimer_udelay(ci_ctimer_inf* i, int delay)
{
  ci_uint32 cycles, end;

  ci_ctimer_frc32(&cycles);

  end =
    delay * (ci_uint32) (i->hz / 1000000) + cycles - (ci_uint32) i->overhead;

  while( ci_int32_lt(cycles, end) )  ci_ctimer_frc32(&cycles);
}


/*! Comment? */
ci_inline void ci_ctimer_ndelay(ci_ctimer_inf* i, int delay)
{
  ci_uint32 cycles, end;

  ci_ctimer_frc32(&cycles);

  end =
    (ci_uint32) (delay * i->hz / 1000000000) + cycles - (ci_uint32) i->overhead;

  while( ci_int32_lt(cycles, end) )  ci_ctimer_frc32(&cycles);
}


/*! Comment? */
extern void ci_ctimer_calibrate(ci_ctimer_inf* i);


/*! for some platforms citools can calculate your processor frequency, otherwise
   you have to supply with the _with_hz() varient 
*/
extern int ci_ctimer_init(ci_ctimer_inf*);

/*! Comment? */
ci_inline void ci_ctimer_init_with_hz(ci_ctimer_inf* i, unsigned hz)
{
  i->hz = hz;
  ci_ctimer_calibrate(i);
}

#endif  /* ifdef CI_HAVE_CTIMER */

#endif  /* __CI_APP_CTIMER_H__ */

/*! \cidoxg_end */
