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
 * This file contains API provided by efhw/eventq.c file.  This file is not
 * designed for use outside of the SFC resource driver.
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

#ifndef __CI_EFHW_EVENTQ_H__
#define __CI_EFHW_EVENTQ_H__

#include <ci/efhw/efhw_types.h>
#include <ci/efhw/eventq_macros.h>

/*! Poll the event queue. */
extern int efhw_keventq_poll(struct efhw_nic *, struct efhw_keventq *);

/*! Callbacks for handling events. */
struct efhw_ev_handler {
	void (*wakeup_fn)(struct efhw_nic *nic, unsigned);
	void (*timeout_fn)(struct efhw_nic *nic, unsigned);
	void (*dmaq_flushed_fn) (struct efhw_nic *, unsigned, int, int);
};

extern int efhw_keventq_ctor(struct efhw_nic *, int instance,
			     struct efhw_keventq *, struct efhw_ev_handler *);
extern void efhw_keventq_dtor(struct efhw_nic *, struct efhw_keventq *);

extern void efhw_handle_txdmaq_flushed(struct efhw_nic *,
				       struct efhw_ev_handler *,
				       efhw_event_t *);
extern void efhw_handle_rxdmaq_flushed(struct efhw_nic *,
				       struct efhw_ev_handler *,
				       efhw_event_t *);
extern void efhw_handle_wakeup_event(struct efhw_nic *,
				     struct efhw_ev_handler *,
				     efhw_event_t *);
extern void efhw_handle_timeout_event(struct efhw_nic *,
				      struct efhw_ev_handler *,
				      efhw_event_t *);

#endif /* __CI_EFHW_EVENTQ_H__ */
