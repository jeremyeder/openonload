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
** <L5_PRIVATE L5_HEADER >
** \author  stg
**  \brief  Efab IP ID allocation mechanism
**   \date  2004/09/22
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_driver_efab */

#ifndef __CI_DRIVER_EFAB_IPID_H__
#define __CI_DRIVER_EFAB_IPID_H__

#include <ci/tools/sysdep.h>
#include <ci/tools/spinlock.h>
#include <ci/internal/ipid.h>  /* symbols shared with UL */

/*! IP ID control structure - precisely one exists in each 
 * TCP Helper Resource Manager struct */
typedef struct {
  ci_irqlock_t lock;				/*! table-wide lock */
  ci_uint8 range[ CI_IPID_BLOCK_COUNT ];	/*!< in-use flag */
#ifndef NDEBUG
  int init;
# define EFAB_IPID_INIT 0x7706EE9B
#endif
} efab_ipid_cb_t;

/*! Allocate a block of IDs from the pool of ID blocks */
extern int efab_ipid_alloc(efab_ipid_cb_t* ipid);

/*! Release a block of IDs previosuly allocated through
 * efab_ipid_alloc(). Base MUST be the base address returned
 * by efab_ipid_alloc().  */
extern int efab_ipid_free(efab_ipid_cb_t* ipid, int base);

ci_inline void
efab_ipid_ctor( efab_ipid_cb_t* ipid )
{
  ci_assert(ipid);
  ci_irqlock_ctor( &ipid->lock );
  memset( ipid->range, 0, sizeof( ipid->range ));
#ifndef NDEBUG
  ipid->init = EFAB_IPID_INIT;
#endif
}

ci_inline void
efab_ipid_dtor( efab_ipid_cb_t* ipid )
{
  ci_assert(ipid);
  ci_assert( ipid->init == EFAB_IPID_INIT );
  ci_irqlock_dtor( &ipid->lock );
#ifndef NDEBUG
  ipid->init = 0;
#endif
}

#endif


/*! \cidoxg_end */
