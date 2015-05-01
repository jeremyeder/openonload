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

/****************************************************************************
 * Driver for Solarflare network controllers -
 *          resource management for Xen backend, OpenOnload, etc
 *           (including support for SFE4001 10GBT NIC)
 *
 * This file provides private buffer table API.  This API is not designed
 * for use outside of SFC resource driver.
 *
 * Copyright 2005-2007: Solarflare Communications Inc,
 *                      9501 Jeronimo Road, Suite 250,
 *                      Irvine, CA 92618, USA
 *
 * Developed and maintained by Solarflare Communications:
 *                      <linux-xen-drivers@solarflare.com>
 *                      <onload-dev@solarflare.com>
 *
 * Certain parts of the driver were implemented by
 *          Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *          OKTET Labs Ltd, Russia,
 *          http://oktetlabs.ru, <info@oktetlabs.ru>
 *          by request of Solarflare Communications
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, incorporated herein by reference.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ****************************************************************************
 */

#ifndef __CI_EFRM_BUFFER_TABLE_H__
#define __CI_EFRM_BUFFER_TABLE_H__

#include <ci/efhw/efhw_types.h>
#include <ci/efrm/debug.h>

struct efrm_bt_collection {
	struct efrm_buffer_table_allocation *allocs;
	/* Number of efrm_buffer_table_allocations we have allocated memory
	 * for - these are not necessarily populated with an actual buffer
	 * table allocation. Whether a particular entry contains actual buffer
	 * table entries can be determined by bta_size.
	 */
	int num_allocs;
};

struct efrm_buffer_table_allocation {
	/* list of blocks, not null-terminated */
	struct efhw_buffer_table_block *bta_blocks;

	/* first used entry in the first block */
	int bta_first_entry_offset;

	/* number of buffer table entries */
	int bta_size;

	/* order of each bt entry */
	int bta_order;
};

/* Manager to keep together similar buffer table allocations.
 * All blocks should have same nic, owner and order.
 * User is also responsible for locking. */
struct efrm_bt_manager {
	/* Block with some free entries */
	struct efhw_buffer_table_block *btm_block;

	/* Owner for all buftable entries we manage */
	int owner;

	/* Order for all buftable entries we manage */
	int order;

	/* Lock to protect btm_block buffer from being released and
	 * btb_free_mask inside shared blocks.
	 * Do not include any efhw operations into protected area,
	 * because efhw ops may sleep. */
	spinlock_t btm_lock;

	/* Number of blocks allocated under this manager */
	atomic_t btm_blocks;
	/* Number of entries allocated under this manager */
	atomic_t btm_entries;
};

/* This function is used for queue-backed bt entries, i.e. for
 * falcon/siena only. */
static inline int
efrm_bt_allocation_base(struct efrm_buffer_table_allocation *a)
{
	EFRM_ASSERT(a->bta_order == 0);
	if (a->bta_size == 0)
		return 0;
	EFRM_ASSERT(a->bta_size <= EFHW_BUFFER_TABLE_BLOCK_SIZE);
	EFRM_ASSERT(a->bta_size + a->bta_first_entry_offset  <=
		    EFHW_BUFFER_TABLE_BLOCK_SIZE);
	return a->bta_first_entry_offset +
		(a->bta_blocks->btb_vaddr >> EFHW_NIC_PAGE_SHIFT);
}

#endif /* __CI_EFRM_BUFFER_TABLE_H__ */
