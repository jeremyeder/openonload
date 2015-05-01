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

/****************************************************************************
 * Driver for Solarflare network controllers -
 *          resource management for Xen backend, OpenOnload, etc
 *
 * This file provides internal API for buffer table manager.
 *
 * Copyright 2012-2012: Solarflare Communications Inc,
 *                      9501 Jeronimo Road, Suite 250,
 *                      Irvine, CA 92618, USA
 *
 * Developed and maintained by Solarflare Communications:
 *                      <linux-xen-drivers@solarflare.com>
 *                      <onload-dev@solarflare.com>
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

#include "bt_manager.h"
#include <ci/driver/efab/hardware.h>

static int
efrm_bt_block_reuse_try(struct efhw_buffer_table_block *block,
			struct efrm_buffer_table_allocation *a)
{
	int i;

	i = ffs(block->btb_free_mask) - 1;
	while (i + a->bta_size <= EFHW_BUFFER_TABLE_BLOCK_SIZE) {
		uint32_t mask = EFHW_BT_BLOCK_RANGE(i, a->bta_size);
		if ((block->btb_free_mask & mask) == mask) {
			a->bta_first_entry_offset = i;
			a->bta_blocks = block;
			block->btb_free_mask &= ~mask;
			return 0;
		}
		i += ffs(~(block->btb_free_mask >> i)) + 1;
	}

	return -ENOMEM;
}

static int
efrm_bt_blocks_alloc(struct efhw_nic *nic,
		     struct efrm_bt_manager *manager,
		     struct efrm_buffer_table_allocation *a)
{
	int i, rc;
	int size = a->bta_size;
	int n_blocks = (size - 1) / EFHW_BUFFER_TABLE_BLOCK_SIZE + 1;
	struct efhw_buffer_table_block *block;

	a->bta_blocks = NULL;
	a->bta_first_entry_offset = 0;

	for (i = 0; i < n_blocks; i++) {
		rc = efhw_nic_buffer_table_alloc(nic, manager->owner,
						 manager->order, &block);
		if (rc != 0)
			goto fail;
		block->btb_next = a->bta_blocks;
		if (size >= EFHW_BUFFER_TABLE_BLOCK_SIZE)
			block->btb_free_mask = 0;
		else
			block->btb_free_mask = EFHW_BT_BLOCK_FREE_ALL &
					~EFHW_BT_BLOCK_RANGE(0, size);
		a->bta_blocks = block;
		size -= EFHW_BUFFER_TABLE_BLOCK_SIZE;
	}

	atomic_add(n_blocks, &manager->btm_blocks);

	return 0;

fail:
	EFRM_ERR_LIMITED("%s: failed size=%d order=%d rc=%d",
			 __FUNCTION__, size, manager->order, rc);
	while ( (block = a->bta_blocks) != NULL) {
		a->bta_blocks = block->btb_next;
		efhw_nic_buffer_table_free(nic, block);
	}
	return rc;
}

void
efrm_bt_blocks_free(struct efhw_nic *nic,
		    struct efrm_buffer_table_allocation *a)
{
	int n = a->bta_size;
	struct efhw_buffer_table_block *block;

	EFRM_ASSERT(a->bta_first_entry_offset == 0);
	while ( (block = a->bta_blocks) != NULL) {
		efhw_nic_buffer_table_clear(nic, block, 0,
				min(n, EFHW_BUFFER_TABLE_BLOCK_SIZE));
		n -= EFHW_BUFFER_TABLE_BLOCK_SIZE;
		a->bta_blocks = block->btb_next;
		efhw_nic_buffer_table_free(nic, block);
	}
	EFRM_DO_DEBUG(a->bta_size = 0);
}

int
efrm_bt_manager_alloc(struct efhw_nic *nic,
		      struct efrm_bt_manager *manager, int size,
		      struct efrm_buffer_table_allocation *a)
{
	int rc;

	a->bta_size = size;
	a->bta_order = manager->order;
	atomic_add(a->bta_size, &manager->btm_entries);

	if (size < EFHW_BUFFER_TABLE_BLOCK_SIZE &&
	    manager->btm_block != NULL) {
		spin_lock_bh(&manager->btm_lock);
		/* Try to satisfy the request from already-allocated
		 * block. */
		if (manager->btm_block != NULL &&
		    efrm_bt_block_reuse_try(manager->btm_block, a) == 0) {
			if (manager->btm_block->btb_free_mask == 0)
				manager->btm_block = NULL;
			spin_unlock_bh(&manager->btm_lock);
			return 0;
		}
		spin_unlock_bh(&manager->btm_lock);
	}

	/* Failed to allocate from already-existing block.
	 * Let's get another block(s)! */
	rc = efrm_bt_blocks_alloc(nic, manager, a);
	if (rc != 0) {
		atomic_sub(a->bta_size, &manager->btm_entries);
		return rc;
	}

	/* Is this new block better than current btm_block? */
	if (size < EFHW_BUFFER_TABLE_BLOCK_SIZE) {
		spin_lock_bh(&manager->btm_lock);
		if (size <= EFHW_BUFFER_TABLE_BLOCK_SIZE / 2 ||
		    ffs(manager->btm_block->btb_free_mask) > size)
			manager->btm_block = a->bta_blocks;
		spin_unlock_bh(&manager->btm_lock);
	}

	return 0;
}

static void
efrm_bt_free_small(struct efhw_nic *nic, struct efrm_bt_manager *manager,
		   struct efrm_buffer_table_allocation *a)
{
	EFRM_ASSERT(a->bta_size < EFHW_BUFFER_TABLE_BLOCK_SIZE);

	efhw_nic_buffer_table_clear(nic, a->bta_blocks,
				    a->bta_first_entry_offset,
				    a->bta_size);

	spin_lock_bh(&manager->btm_lock);
	a->bta_blocks->btb_free_mask |=
		EFHW_BT_BLOCK_RANGE(a->bta_first_entry_offset,
				    a->bta_size);

	/* if the block is in use, do nothing */
	if (a->bta_blocks->btb_free_mask != EFHW_BT_BLOCK_FREE_ALL) {
		spin_unlock_bh(&manager->btm_lock);
		return;
	}

	/* free block: remove link and free */
	if (a->bta_blocks == manager->btm_block)
		manager->btm_block = NULL;
	spin_unlock_bh(&manager->btm_lock);

	efhw_nic_buffer_table_free(nic, a->bta_blocks);
	atomic_dec(&manager->btm_blocks);
}

static void
efrm_bt_realloc_free(struct efhw_nic *nic,
		     struct efrm_bt_manager *manager,
		     struct efrm_buffer_table_allocation *a,
		     struct efhw_buffer_table_block *failed_block)
{
	int n = a->bta_size;
	struct efhw_buffer_table_block *block = a->bta_blocks;
	struct efhw_buffer_table_block *next;

	while (block != failed_block) {
		next = block->btb_next;
		efhw_nic_buffer_table_free(nic, block);
		block = next;
		n -= EFHW_BUFFER_TABLE_BLOCK_SIZE;
		atomic_dec(&manager->btm_blocks);
	}
	block = failed_block->btb_next;
	efhw_nic_buffer_table_free(nic, failed_block);
	atomic_dec(&manager->btm_blocks);

	while (n > 0) {
		next = block->btb_next;
		/* we ignore rc from realloc(): free() should be called
		 * in any case */
		efhw_nic_buffer_table_realloc(nic, manager->owner,
					      manager->order, block);
		efhw_nic_buffer_table_free(nic, block);
		block = next;
		n -= EFHW_BUFFER_TABLE_BLOCK_SIZE;
		atomic_dec(&manager->btm_blocks);
	}
}

int
efrm_bt_manager_realloc(struct efhw_nic *nic,
			struct efrm_bt_manager *manager,
			struct efrm_buffer_table_allocation *a)
{
	int n = a->bta_size;
	int rc = 0;
	struct efhw_buffer_table_block *block = a->bta_blocks;

	EFRM_DO_DEBUG(
		if (a->bta_size > EFHW_BUFFER_TABLE_BLOCK_SIZE)
			EFRM_ASSERT(a->bta_first_entry_offset == 0);
		else
			EFRM_ASSERT(a->bta_first_entry_offset + a->bta_size
				    <= EFHW_BUFFER_TABLE_BLOCK_SIZE);
	)

	spin_lock_bh(&manager->btm_lock);
	if (manager->btm_block != NULL)
		manager->btm_block = NULL;
	spin_unlock_bh(&manager->btm_lock);

	do {
		uint32_t mask = EFHW_BT_BLOCK_RANGE(
				a->bta_first_entry_offset,
				min(n, EFHW_BUFFER_TABLE_BLOCK_SIZE));

		/* Check if the block was re-allocated */
		spin_lock_bh(&manager->btm_lock);
		if ((block->btb_free_mask & mask) == 0) {
			block->btb_free_mask = EFHW_BT_BLOCK_FREE_ALL & ~mask;
			spin_unlock_bh(&manager->btm_lock);
			rc = efhw_nic_buffer_table_realloc(nic,
							   manager->owner,
							   manager->order,
							   block);
			if( rc != 0 )
				goto fail;
		}
		else {
			EFRM_ASSERT((block->btb_free_mask & mask) == mask);
			block->btb_free_mask &= ~mask;
			spin_unlock_bh(&manager->btm_lock);
		}
		n -= EFHW_BUFFER_TABLE_BLOCK_SIZE;
		block = block->btb_next;
	} while (n > 0);

	return 0;

fail:
	EFRM_ERR("%s ERROR: failed to re-allocate buffer table entries "
		 "after reset size=%d", __func__, a->bta_size);
	if (a->bta_size <= EFHW_BUFFER_TABLE_BLOCK_SIZE)
		efrm_bt_free_small(nic, manager, a);
	else
		efrm_bt_realloc_free(nic, manager, a, block);
	a->bta_size = 0;
	EFRM_DO_DEBUG(a->bta_blocks = NULL;)
	return rc;
}

void
efrm_bt_manager_free(struct efhw_nic *nic, struct efrm_bt_manager *manager,
		     struct efrm_buffer_table_allocation *a)
{
	EFRM_ASSERT(a->bta_order == manager->order);

	atomic_sub(a->bta_size, &manager->btm_entries);

	if (a->bta_size < EFHW_BUFFER_TABLE_BLOCK_SIZE) {
		efrm_bt_free_small(nic, manager, a);
		a->bta_size = 0;
		return;
	}

	atomic_sub((a->bta_size - 1) / EFHW_BUFFER_TABLE_BLOCK_SIZE + 1,
		   &manager->btm_blocks);
	efrm_bt_blocks_free(nic, a);
	a->bta_size = 0;
}

void
efrm_bt_nic_set(struct efhw_nic *nic, struct efrm_buffer_table_allocation *a,
		dma_addr_t *dma_addrs)
{
	int n = a->bta_size;
	struct efhw_buffer_table_block *block = a->bta_blocks;

	EFRM_DO_DEBUG(
		if (a->bta_size > EFHW_BUFFER_TABLE_BLOCK_SIZE)
			EFRM_ASSERT(a->bta_first_entry_offset == 0);
		else
			EFRM_ASSERT(a->bta_first_entry_offset + a->bta_size
				    <= EFHW_BUFFER_TABLE_BLOCK_SIZE);
	)

	do {
		efhw_nic_buffer_table_set(nic, block,
				a->bta_first_entry_offset,
				min(n, EFHW_BUFFER_TABLE_BLOCK_SIZE),
				dma_addrs + (a->bta_size - n));
		n -= EFHW_BUFFER_TABLE_BLOCK_SIZE;
		block = block->btb_next;
	} while (n > 0);
}

