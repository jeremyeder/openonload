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
 * This file contains definition of the public type struct linux_efhw_nic.
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

#ifndef __CI_DRIVER_RESOURCE_LINUX_RESOURCE__
#define __CI_DRIVER_RESOURCE_LINUX_RESOURCE__

#ifndef __KERNEL__
# error Silly
#endif

#include <ci/efrm/efrm_nic.h>
#include <linux/interrupt.h>


/************************************************************************
 * Per-nic structure in the resource driver                             *
 ************************************************************************/

struct linux_efhw_nic {
	struct efrm_nic efrm_nic;

	/* Driverlink device context */
	struct efx_dl_device *dl_device;

	/*! Callbacks for driverlink, when needed. */
	struct efx_dl_callbacks *dl_callbacks;

	/*! Event handlers. */
	struct efhw_ev_handler *ev_handlers;

};

#define linux_efhw_nic(_efhw_nic)					\
  container_of(_efhw_nic, struct linux_efhw_nic, efrm_nic.efhw_nic)

#endif /* __CI_DRIVER_RESOURCE_LINUX_RESOURCE__ */
