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

/****************************************************************************
 * Driver for Solarflare network controllers -
 *          resource management for Xen backend, OpenOnload, etc
 *           (including support for SFE4001 10GBT NIC)
 *
 * This file provides EtherFabric NIC - EFXXXX (aka Falcon) specific
 * definitions.
 *
 * Copyright 2005-2007: Solarflare Communications Inc,
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

#ifndef __CI_DRIVER_EFAB_HARDWARE_FALCON_H__
#define __CI_DRIVER_EFAB_HARDWARE_FALCON_H__

/*----------------------------------------------------------------------------
 * Compile options
 *---------------------------------------------------------------------------*/

/* Falcon has an 8K maximum page size. */
#define FALCON_MAX_PAGE_SIZE EFHW_8K

/* include the register definitions */
#ifdef USE_OLD_HWDEFS
#include <ci/driver/efab/hardware/falcon/falcon_mac.h>
#endif
#include <ci/driver/efab/hardware/falcon/falcon_desc.h>
#include <ci/driver/efab/hardware/falcon/falcon_event.h>
#include <ci/driver/efab/hardware/falcon/falcon_grmon.h>
#include <ci/driver/efab/hardware/falcon/falcon_xgrmon.h>
#include <ci/driver/efab/hardware/falcon/falcon_intr_vec.h>

#define FALCON_DMA_TX_DESC_BYTES	8
#define FALCON_DMA_RX_PHYS_DESC_BYTES	8
#define FALCON_DMA_RX_BUF_DESC_BYTES	4

/* ---- efhw_event_t helpers --- */

/*!\ TODO look at whether there is an efficiency gain to be had by
  treating the event codes to 32bit masks as is done for EF1

  These masks apply to the full 64 bits of the event to extract the
  event code - followed by the common event codes to expect
 */
#define __FALCON_OPEN_MASK(WIDTH)  ((((uint64_t)1) << (WIDTH)) - 1)
#define FALCON_EVENT_CODE_MASK \
	(__FALCON_OPEN_MASK(EV_CODE_WIDTH) << EV_CODE_LBN)
#define FALCON_EVENT_EV_Q_ID_MASK \
	(__FALCON_OPEN_MASK(DRIVER_EV_EVQ_ID_WIDTH) << DRIVER_EV_EVQ_ID_LBN)
#define FALCON_EVENT_TX_FLUSH_Q_ID_MASK \
	(__FALCON_OPEN_MASK(DRIVER_EV_TX_DESCQ_ID_WIDTH) << \
	 DRIVER_EV_TX_DESCQ_ID_LBN)
#define FALCON_EVENT_RX_FLUSH_Q_ID_MASK \
	(__FALCON_OPEN_MASK(DRIVER_EV_RX_DESCQ_ID_WIDTH) << \
	 DRIVER_EV_RX_DESCQ_ID_LBN)
#define FALCON_EVENT_RX_FLUSH_FAIL_MASK \
	(__FALCON_OPEN_MASK(DRIVER_EV_RX_FLUSH_FAIL_WIDTH) << \
	 DRIVER_EV_RX_FLUSH_FAIL_LBN)
#define FALCON_EVENT_DRV_SUBCODE_MASK \
	(__FALCON_OPEN_MASK(DRIVER_EV_SUB_CODE_WIDTH) << \
	 DRIVER_EV_SUB_CODE_LBN)

#define FALCON_EVENT_FMT         "[ev:%x:%08x:%08x]"
#define FALCON_EVENT_PRI_ARG(e) \
	((unsigned)(((le64_to_cpu((e).u64)) & FALCON_EVENT_CODE_MASK) >> EV_CODE_LBN)), \
    ((unsigned)((le64_to_cpu((e).u64)) >> 32)),                         \
    ((unsigned)(((le64_to_cpu((e).u64)) & 0xFFFFFFFF)))

#define FALCON_EVENT_CODE(evp)		(le64_to_cpu((evp)->u64) &      \
                                   FALCON_EVENT_CODE_MASK)
#define FALCON_EVENT_WAKE_EVQ_ID(evp) \
	((le64_to_cpu((evp)->u64) &                       \
    FALCON_EVENT_EV_Q_ID_MASK) >> DRIVER_EV_EVQ_ID_LBN)
#define FALCON_EVENT_TX_FLUSH_Q_ID(evp) \
	((le64_to_cpu((evp)->u64) & FALCON_EVENT_TX_FLUSH_Q_ID_MASK) >> \
	 DRIVER_EV_TX_DESCQ_ID_LBN)
#define FALCON_EVENT_RX_FLUSH_Q_ID(evp) \
	((le64_to_cpu((evp)->u64) & FALCON_EVENT_RX_FLUSH_Q_ID_MASK) >> \
	 DRIVER_EV_RX_DESCQ_ID_LBN)
#define FALCON_EVENT_RX_FLUSH_FAIL(evp) \
	((le64_to_cpu((evp)->u64) & FALCON_EVENT_RX_FLUSH_FAIL_MASK) >> \
	 DRIVER_EV_RX_FLUSH_FAIL_LBN)
#define FALCON_EVENT_DRIVER_SUBCODE(evp) \
	(((le64_to_cpu((evp)->u64)) & FALCON_EVENT_DRV_SUBCODE_MASK) >> \
	 DRIVER_EV_SUB_CODE_LBN)

#define FALCON_EVENT_CODE_CHAR	((uint64_t)DRIVER_EV_DECODE << EV_CODE_LBN)
#define FALCON_EVENT_CODE_SW	((uint64_t)DRV_GEN_EV_DECODE << EV_CODE_LBN)


/* so this is the size in bytes of an awful lot of things */
#define FALCON_REGISTER128          (16)

/* we define some unique dummy values as a debug aid */
#define FALCON_ATOMIC_BASE		0xdeadbeef00000000ULL
#define FALCON_ATOMIC_UPD_REG		(FALCON_ATOMIC_BASE | 0x1)
#define FALCON_ATOMIC_PTR_TBL_REG	(FALCON_ATOMIC_BASE | 0x2)
#define FALCON_ATOMIC_SRPM_UDP_EVQ_REG	(FALCON_ATOMIC_BASE | 0x3)
#define FALCON_ATOMIC_RX_FLUSH_DESCQ	(FALCON_ATOMIC_BASE | 0x4)
#define FALCON_ATOMIC_TX_FLUSH_DESCQ	(FALCON_ATOMIC_BASE | 0x5)
#define FALCON_ATOMIC_INT_EN_REG	(FALCON_ATOMIC_BASE | 0x6)
#define FALCON_ATOMIC_TIMER_CMD_REG	(FALCON_ATOMIC_BASE | 0x7)
#define FALCON_ATOMIC_PACE_REG		(FALCON_ATOMIC_BASE | 0x8)
#define FALCON_ATOMIC_INT_ACK_REG	(FALCON_ATOMIC_BASE | 0x9)
/* XXX It crashed with odd value in FALCON_ATOMIC_INT_ADR_REG */
#define FALCON_ATOMIC_INT_ADR_REG	(FALCON_ATOMIC_BASE | 0xa)

/*----------------------------------------------------------------------------
 *
 * PCI control blocks for Falcon -
 *          (P) primary is for NET
 *          (S) secondary is for CHAR
 *
 *---------------------------------------------------------------------------*/

#define FALCON_P_CTR_AP_BAR	2
#define FALCON_S_CTR_AP_BAR	0
#define FALCON_S_DEVID		0x6703


/*----------------------------------------------------------------------------
 *
 * Falcon constants
 *
 *---------------------------------------------------------------------------*/

/* Note: the following constants have moved to values in struct efhw_nic
 * because they are different between Falcon and Siena:
 *   FALCON_EVQ_TBL_NUM  ->  nic->num_evqs
 *   FALCON_DMAQ_NUM     ->  nic->num_dmaqs
 *   FALCON_TIMERS_NUM   ->  nic->num_times
 * These replacement constants are used as sanity checks in assertions in
 * certain functions that don't have access to struct efhw_nic.  They may
 * catch some errors but do *not* guarantee a valid value for Siena.
 */
#define FALCON_DMAQ_NUM_SANITY          (EFHW_4K)
#define FALCON_EVQ_TBL_NUM_SANITY       (EFHW_4K)
#define FALCON_TIMERS_NUM_SANITY        (EFHW_4K)

/* This value is an upper limit on the total number of filter table
 * entries.  The actual size of filter table is determined at runtime, as
 * it can vary.
 */
#define FALCON_FILTER_TBL_NUM		(EFHW_8K)

/* max number of buffers which can be pushed before commiting */
#define FALCON_BUFFER_UPD_MAX		(128)

/* This is the buffer size falcon will assume when an RX queue is in
 * "split" mode.  Granularity is 32 bytes.
 *
 * This value has been chosen for the onload stack.  i.e. 2k - meta-data
 * prefix size.
 */
#define FALCON_RX_USR_BUF_SIZE		(2048 - 256)

#define FALCON_EVQ_RPTR_REG_P0		0x400

/*----------------------------------------------------------------------------
 *
 * Falcon requires user-space descriptor pushes to be:
 *    dword[0-2]; wiob(); dword[3]
 *
 * Driver register access must be locked against other threads from
 * the same driver but can be in any order: i.e dword[0-3]; wiob()
 *
 * The following helpers ensure that valid dword orderings are exercised
 *
 *---------------------------------------------------------------------------*/

/* A union to allow writting 64bit values as 32bit values, without
 * hitting the compilers aliasing rules. We hope the compiler
 * optimises away the copy's anyway.  Note that this definition is
 * also being used by ef10.h.  Maybe move it to another file? */
union __u64to32 {
	uint64_t u64;
	struct {
#ifdef EFHW_IS_LITTLE_ENDIAN
		uint32_t a;
		uint32_t b;
#else
		uint32_t b;
		uint32_t a;
#endif
	} s;
};

/* Ensure DW3 is written last. Outer locking cannot be relied upon to provide
 * a write barrier
 */
static inline void
falcon_write_ddd_d(volatile char __iomem *kva,
		   uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3)
{
	writel(d0, kva + 0);
	writel(d1, kva + 4);
	writel(d2, kva + 8);
	wmb();
	writel(d3, kva + 12);
	wmb();
}

/* Ensure DW3 is written last. Outer locking cannot be relied upon to provide
 * a write barrier
 */
static inline void falcon_write_q(volatile char __iomem *kva, uint64_t q)
{
	union __u64to32 u;
	u.u64 = q;

	writel(u.s.a, kva);
	wmb();
	writel(u.s.b, kva + 4);
	wmb();
}

static inline void falcon_read_q(volatile char __iomem *addr, uint64_t *q0)
{
	/* It is essential that we read dword0 first, so that
	 * the shadow register is updated with the latest value
	 * and we get a self consistent value.
	 */
	union __u64to32 u;
	/* The CPU must always waits for a read to complete so locked sequences
	 * of reads cannot be interleaved. Lock is outside this function.
	 */
	u.s.a = readl(addr);
	rmb(); /* to stop compiler/CPU re-ordering these two reads*/
	u.s.b = readl(addr + 4);
	rmb(); /* just be safe: so falcon_read_q() can be composed */

	*q0 = u.u64;
}

static inline void
falcon_write_qq(volatile char __iomem *kva, uint64_t q0, uint64_t q1)
{
	union __u64to32 u;
	writeq(q0, kva + 0);
	u.u64 = q1;
	writel(u.s.a, kva + 8);
	wmb();
	writel(u.s.b, kva + 12);
	wmb();
}

static inline void
falcon_read_qq(volatile char __iomem *addr, uint64_t *q0, uint64_t *q1)
{
	union __u64to32 u;
	u.s.a = readl(addr);
	rmb();
	u.s.b = readl(addr+4);
	*q0 = u.u64;
	*q1 = readq(addr+8);
	rmb();
}



/*----------------------------------------------------------------------------
 *
 * Buffer virtual addresses (4K buffers)
 *
 *---------------------------------------------------------------------------*/

/* Form a buffer virtual address from buffer ID and offset.  If the offset
** is larger than the buffer size, then the buffer indexed will be
** calculated appropriately.  It is the responsibility of the caller to
** ensure that they have valid buffers programmed at that address.
*/
#define FALCON_VADDR_8K_S	(13)
#define FALCON_VADDR_4K_S	(12)
#define FALCON_VADDR_M		0xfffff	/* post shift mask  */

#define FALCON_BUFFER_8K_ADDR(id, off)	(((id) << FALCON_VADDR_8K_S) + (off))
#define FALCON_BUFFER_8K_PAGE(vaddr) \
	(((vaddr) >> FALCON_VADDR_8K_S) & FALCON_VADDR_M)
#define FALCON_BUFFER_8K_OFF(vaddr) \
	((vaddr) & __FALCON_MASK32(FALCON_VADDR_8K_S))

#define FALCON_BUFFER_4K_ADDR(id, off)	(((id) << FALCON_VADDR_4K_S) + (off))
#define FALCON_BUFFER_4K_PAGE(vaddr) \
	(((vaddr) >> FALCON_VADDR_4K_S) & FALCON_VADDR_M)
#define FALCON_BUFFER_4K_OFF(vaddr) \
	((vaddr) & __FALCON_MASK32(FALCON_VADDR_4K_S))


/*----------------------------------------------------------------------------
 *
 * DMA Queue helpers
 *
 *---------------------------------------------------------------------------*/

/* iSCSI queue for A1; see bug 5427 for more details. */
#define FALCON_A1_ISCSI_DMAQ 4

/*! returns an address within a bar of the TX DMA doorbell */
static inline uint falcon_tx_dma_page_addr(uint dmaq_idx)
{
	uint page;
#ifdef HEADER_REVIEW
#warning TBD this needs more clean up; the function does not get a version
#warning the function makes an index range check; this is device dependent
#endif

	EFHW_ASSERT((((FR_AB_TX_DESC_UPD_REGP123_OFST) & (EFHW_8K - 1)) ==
		     (((FR_AA_TX_DESC_UPD_REGP0_OFST) & (EFHW_8K - 1)))));

	EFHW_ASSERT(dmaq_idx < FALCON_DMAQ_NUM_SANITY);

	if (dmaq_idx < 1024)
		page = FR_AA_TX_DESC_UPD_REGP0_OFST + ((dmaq_idx - 4) * EFHW_8K);
	else
		page =
		    FR_AB_TX_DESC_UPD_REGP123_OFST +
		    ((dmaq_idx - 1024) * EFHW_8K);

	return page;
}

/*! returns an address within a bar of the RX DMA doorbell */
static inline uint falcon_rx_dma_page_addr(uint dmaq_idx)
{
	uint page;
#ifdef HEADER_REVIEW
#warning TBD this needs more clean up; the function does not get a version
#warning the function makes an index range check; this is device dependent
#endif

	EFHW_ASSERT((((FR_AB_RX_DESC_UPD_REGP123_OFST) & (EFHW_8K - 1)) ==
		     ((FR_AA_RX_DESC_UPD_REGP0_OFST) & (EFHW_8K - 1))));

	EFHW_ASSERT(dmaq_idx < FALCON_DMAQ_NUM_SANITY);

	if (dmaq_idx < 1024)
		page = FR_AA_RX_DESC_UPD_REGP0_OFST + ((dmaq_idx - 4) * EFHW_8K);
	else
		page =
		    FR_AB_RX_DESC_UPD_REGP123_OFST +
		    ((dmaq_idx - 1024) * EFHW_8K);

	return page;
}

/*! "page"=NIC-dependent register set size */
#define FALCON_DMA_PAGE_MASK  (EFHW_8K-1)

/*! returns an address within a bar of the start of the "page"
    containing the TX DMA doorbell */
static inline int falcon_tx_dma_page_base(uint dma_idx)
{
	return falcon_tx_dma_page_addr(dma_idx) & ~FALCON_DMA_PAGE_MASK;
}

/*! returns an address within a bar of the start of the "page"
    containing the RX DMA doorbell */
static inline int falcon_rx_dma_page_base(uint dma_idx)
{
	return falcon_rx_dma_page_addr(dma_idx) & ~FALCON_DMA_PAGE_MASK;
}

/*! returns an offset within a "page" of the TX DMA doorbell */
static inline int falcon_tx_dma_page_offset(uint dma_idx)
{
	return falcon_tx_dma_page_addr(dma_idx) & FALCON_DMA_PAGE_MASK;
}

/*! returns an offset within a "page" of the RX DMA doorbell */
static inline int falcon_rx_dma_page_offset(uint dma_idx)
{
	return falcon_rx_dma_page_addr(dma_idx) & FALCON_DMA_PAGE_MASK;
}

/*----------------------------------------------------------------------------
 *
 * Events
 *
 *---------------------------------------------------------------------------*/

#define FALCON_A_EVQ_CHAR      (4)	/* min evq accessible via char bar */

/* default DMA-Q sizes */
#define FALCON_DMA_Q_DEFAULT_TX_SIZE  512

#define FALCON_DMA_Q_DEFAULT_RX_SIZE  512

#define FALCON_DMA_Q_DEFAULT_MMAP \
	(FALCON_DMA_Q_DEFAULT_TX_SIZE * (FALCON_DMA_TX_DESC_BYTES * 2))

/*----------------------------------------------------------------------------
 *
 * DEBUG - Analyser trigger
 *
 *---------------------------------------------------------------------------*/

static inline void
falcon_deadbeef(volatile char __iomem *efhw_kva, unsigned what)
{
	writel(what, efhw_kva + 0x300);
	wmb();
}
#endif /* __CI_DRIVER_EFAB_HARDWARE_FALCON_H__ */
/*! \cidoxg_end */
