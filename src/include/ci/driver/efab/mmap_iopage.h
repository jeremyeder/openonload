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


#ifndef __CI_DRIVER_EFAB_MMAP_IOPAGE_H__
#define __CI_DRIVER_EFAB_MMAP_IOPAGE_H__

#include <ci/efhw/iopage.h>

/*--------------------------------------------------------------------
 *
 * memory management
 *
 *--------------------------------------------------------------------*/

/*! Comment? */
extern int
ci_mmap_bar(struct efhw_nic* nic, off_t base, size_t len, void* opaque,
            int* map_num, unsigned long* offset);

extern int
ci_mmap_iopage(struct efhw_iopage* p, void* opaque, int* map_num,
	       unsigned long* offset);

extern int
ci_mmap_iopages(struct efhw_iopages* p, unsigned offset, unsigned max_bytes,
		unsigned long* bytes, void* opaque,
		int* map_num, unsigned long* p_offset);



#endif /* __CI_DRIVER_EFAB_MMAP_IOPAGE_H__ */
