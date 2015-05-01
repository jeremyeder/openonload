/*
** Copyright 2005-2014  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of version 2.1 of the GNU Lesser General Public
** License as published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
*/

/****************************************************************************
 * Copyright 2002-2005: Level 5 Networks Inc.
 * Copyright 2005-2008: Solarflare Communications Inc,
 *                      9501 Jeronimo Road, Suite 250,
 *                      Irvine, CA 92618, USA
 *
 * Maintained by Solarflare Communications
 *  <linux-xen-drivers@solarflare.com>
 *  <onload-dev@solarflare.com>
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

/*
 * \author  djr
 *  \brief  Initialisation of VIs.
 *   \date  2007/06/08
 */

#include "ef_vi_internal.h"
#include "efch_intf_ver.h"
#include <onload/version.h>
#include <etherfabric/init.h>


#define EF_VI_STATE_BYTES(rxq_sz, txq_sz)			\
	(sizeof(ef_vi_state) + (rxq_sz) * sizeof(uint32_t)	\
	 + (txq_sz) * sizeof(uint32_t))

int ef_vi_calc_state_bytes(int rxq_sz, int txq_sz)
{
	EF_VI_BUG_ON(rxq_sz != 0 && ! EF_VI_IS_POW2(rxq_sz));
	EF_VI_BUG_ON(txq_sz != 0 && ! EF_VI_IS_POW2(txq_sz));

	return EF_VI_STATE_BYTES(rxq_sz, txq_sz);
}


int ef_vi_state_bytes(ef_vi* vi)
{
	int rxq_sz = 0, txq_sz = 0;
	if( vi->vi_rxq.mask )
		rxq_sz = vi->vi_rxq.mask + 1;
	if( vi->vi_txq.mask )
		txq_sz = vi->vi_txq.mask + 1;

	EF_VI_BUG_ON(rxq_sz != 0 && ! EF_VI_IS_POW2(rxq_sz));
	EF_VI_BUG_ON(txq_sz != 0 && ! EF_VI_IS_POW2(txq_sz));

	return EF_VI_STATE_BYTES(rxq_sz, txq_sz);
}


void ef_vi_init_state(ef_vi* vi)
{
	ef_vi_reset_rxq(vi);
	ef_vi_reset_txq(vi);
	/* NB. Must not clear the ring as it may already have an
	 * initialisation event in it.
	 */
	ef_vi_reset_evq(vi, 0);
}


int ef_vi_add_queue(ef_vi* evq_vi, ef_vi* add_vi)
{
	int q_label;
	if (evq_vi->vi_qs_n == EF_VI_MAX_QS)
		return -EBUSY;
	q_label = evq_vi->vi_qs_n++;
	EF_VI_BUG_ON(evq_vi->vi_qs[q_label] != NULL);
	evq_vi->vi_qs[q_label] = add_vi;
	return q_label;
}


void ef_vi_set_stats_buf(ef_vi* vi, ef_vi_stats* s)
{
	vi->vi_stats = s;
}


void ef_vi_set_tx_push_threshold(ef_vi* vi, unsigned threshold)
{
	if( vi->nic_type.arch != EF_VI_ARCH_FALCON )
		vi->tx_push_thresh = threshold;
}


const char* ef_vi_version_str(void)
{
	return ONLOAD_VERSION;
}


const char* ef_vi_driver_interface_str(void)
{
	return EFCH_INTF_VER;
}


int ef_vi_rxq_reinit(ef_vi* vi, ef_vi_reinit_callback cb, void* cb_arg)
{
  ef_vi_state* state = vi->ep_state;
  int di;
  
  while( state->rxq.removed < state->rxq.added ) {
    di = state->rxq.removed & vi->vi_rxq.mask;
    BUG_ON(vi->vi_rxq.ids[di] == EF_REQUEST_ID_MASK);
    (*cb)(vi->vi_rxq.ids[di], cb_arg);
    vi->vi_rxq.ids[di] = EF_REQUEST_ID_MASK;
    ++state->rxq.removed;
  }

  state->rxq.added = state->rxq.removed = state->rxq.prev_added = 0;
  state->rxq.in_jumbo = 0;
  state->rxq.bytes_acc = 0;

  return 0;
}


int ef_vi_txq_reinit(ef_vi* vi, ef_vi_reinit_callback cb, void* cb_arg)
{
  ef_vi_state* state = vi->ep_state;
  int di;

  while( state->txq.removed < state->txq.added ) {
    di = state->txq.removed & vi->vi_txq.mask;
    if( vi->vi_txq.ids[di] != EF_REQUEST_ID_MASK )
      (*cb)(vi->vi_txq.ids[di], cb_arg);
    vi->vi_txq.ids[di] = EF_REQUEST_ID_MASK;
    ++state->txq.removed;
  }

  state->txq.previous = state->txq.added = state->txq.removed = 0;

  return 0;
}


int ef_vi_evq_reinit(ef_vi* vi)
{
  memset(vi->evq_base, (char)0xff, vi->evq_mask + 1);
  vi->ep_state->evq.evq_ptr = 0;
  return 0;
}


/**********************************************************************
 * ef_vi_init*
 */

static int rx_desc_bytes(struct ef_vi* vi)
{
  switch( vi->nic_type.arch ) {
  case EF_VI_ARCH_FALCON:
    return (vi->vi_flags & EF_VI_RX_PHYS_ADDR) ? 8 : 4;
  case EF_VI_ARCH_EF10:
    return 8;
  default:
    EF_VI_BUG_ON(1);
    return 8;
  }
}


int ef_vi_rx_ring_bytes(struct ef_vi* vi)
{
	EF_VI_ASSERT(vi->inited & EF_VI_INITED_RXQ);
	return (vi->vi_rxq.mask + 1) * rx_desc_bytes(vi);
}


int ef_vi_init(struct ef_vi* vi, int arch, int variant, int revision,
	       unsigned ef_vi_flags, ef_vi_state* state)
{
	memset(vi, 0, sizeof(*vi));
	/* vi->vi_qs_n = 0; */
	/* vi->inited = 0; */
	/* vi->vi_i = 0; */
	vi->nic_type.arch = arch;
	vi->nic_type.variant = variant;
	vi->nic_type.revision = revision;
	vi->vi_flags = (enum ef_vi_flags) ef_vi_flags;
	vi->ep_state = state;
	/* vi->vi_stats = NULL; */
	/* vi->io = NULL; */
	/* vi->linked_pio = NULL; */
	switch( arch ) {
	case EF_VI_ARCH_FALCON:
		falcon_vi_init(vi);
		break;
	case EF_VI_ARCH_EF10:
		ef10_vi_init(vi);
		break;
	default:
		return -EINVAL;
	}
	vi->inited |= EF_VI_INITED_NIC;
	return 0;
}


void ef_vi_init_io(struct ef_vi* vi, void* io_area)
{
	EF_VI_BUG_ON(vi->inited & EF_VI_INITED_IO);
	EF_VI_BUG_ON(io_area == NULL);
	vi->io = io_area;
	vi->inited |= EF_VI_INITED_IO;
}


void ef_vi_init_rxq(struct ef_vi* vi, int ring_size, void* descriptors,
		    void* ids, int prefix_len)
{
	EF_VI_BUG_ON(vi->inited & EF_VI_INITED_RXQ);
	EF_VI_BUG_ON(ring_size & (ring_size - 1)); /* not power-of-2 */
	vi->vi_rxq.mask = ring_size - 1;
	vi->vi_rxq.descriptors = descriptors;
	vi->vi_rxq.ids = ids;
	vi->rx_prefix_len = prefix_len;
	vi->inited |= EF_VI_INITED_RXQ;
}


void ef_vi_init_txq(struct ef_vi* vi, int ring_size, void* descriptors,
		    void* ids)
{
	EF_VI_BUG_ON(vi->inited & EF_VI_INITED_TXQ);
	vi->vi_txq.mask = ring_size - 1;
	vi->vi_txq.descriptors = descriptors;
	vi->vi_txq.ids = ids;
        if( vi->nic_type.arch == EF_VI_ARCH_FALCON )
          vi->tx_push_thresh = 1;
        else
          vi->tx_push_thresh = 16;
	if( vi->vi_flags & EF_VI_TX_PUSH_DISABLE )
		vi->tx_push_thresh = 0;
	if( (vi->vi_flags & EF_VI_TX_PUSH_ALWAYS) && 
	    vi->nic_type.arch != EF_VI_ARCH_FALCON )
		vi->tx_push_thresh = (unsigned) -1;
	vi->inited |= EF_VI_INITED_TXQ;
}


void ef_vi_init_evq(struct ef_vi* vi, int ring_size, void* event_ring)
{
	EF_VI_BUG_ON(vi->inited & EF_VI_INITED_EVQ);
	vi->evq_mask = ring_size * 8 - 1;
	vi->evq_base = event_ring;
	vi->inited |= EF_VI_INITED_EVQ;
}


void ef_vi_init_timer(struct ef_vi* vi, int timer_quantum_ns)
{
	vi->timer_quantum_ns = timer_quantum_ns;
	vi->inited |= EF_VI_INITED_TIMER;
}


void ef_vi_init_rx_timestamping(struct ef_vi* vi, int rx_ts_correction)
{
	vi->rx_ts_correction = rx_ts_correction;
	vi->inited |= EF_VI_INITED_RX_TIMESTAMPING;
}


void ef_vi_init_tx_timestamping(struct ef_vi* vi)
{
	vi->inited |= EF_VI_INITED_TX_TIMESTAMPING;
}


void ef_vi_init_out_flags(struct ef_vi* vi, unsigned flags)
{
	vi->inited |= EF_VI_INITED_OUT_FLAGS;
	vi->vi_out_flags = flags;
}


void ef_vi_reset_rxq(struct ef_vi* vi)
{
	ef_vi_rxq_state* qs = &vi->ep_state->rxq;
	qs->prev_added = 0;
	qs->added = 0;
	qs->removed = 0;
	qs->in_jumbo = 0;
	qs->bytes_acc = 0;
	if( vi->vi_rxq.mask ) {
		int i;
		for( i = 0; i <= vi->vi_rxq.mask; ++i )
			vi->vi_rxq.ids[i] = EF_REQUEST_ID_MASK;
	}
}


void ef_vi_reset_txq(struct ef_vi* vi)
{
	ef_vi_txq_state* qs = &vi->ep_state->txq;
	qs->previous = 0;
	qs->added = 0;
	qs->removed = 0;
	qs->ts_nsec = EF_VI_TX_TIMESTAMP_TS_NSEC_INVALID;
	if( vi->vi_txq.mask ) {
		int i;
		for( i = 0; i <= vi->vi_txq.mask; ++i )
			vi->vi_txq.ids[i] = EF_REQUEST_ID_MASK;
	}
}


void ef_vi_reset_evq(struct ef_vi* vi, int clear_ring)
{
	if( clear_ring )
		memset(vi->evq_base, (char) 0xff, vi->evq_mask + 1);
	vi->ep_state->evq.evq_ptr = 0;
	vi->ep_state->evq.sync_timestamp_synchronised = 0;
	vi->ep_state->evq.sync_timestamp_major = ~0u;
	vi->ep_state->evq.sync_flags = 0;
}
