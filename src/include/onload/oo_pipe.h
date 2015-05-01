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


#ifndef __ONLOAD_OO_PIPE_H__
#define __ONLOAD_OO_PIPE_H__

#if !CI_CFG_USERSPACE_PIPE
#error "Do not include oo_pipe.h when pipe is not enabled"
#endif

#define oo_pipe_data_len(_p) \
  ((_p)->bytes_added - (_p)->bytes_removed)

/* The write pointer is invalidated when a pipe is filled, but to avoid races
 * it is not reset when space becomes available, and so we must check for space
 * explicitly when the pipe is marked as being full. */
#define oo_pipe_has_space(_p) \
  ((_p)->write_ptr.pp_wait != OO_ACCESS_ONCE((_p)->read_ptr.pp))

/* A pipe is considered writable (for, e.g., select()) even if further buffers
 * would have to be allocated. */
#define oo_pipe_is_writable(_p) (oo_pipe_has_space(_p) || \
                                 (_p)->bufs_num < (_p)->bufs_max)


#ifdef __KERNEL__
void oo_pipe_wake_peer(ci_netif* ni, struct oo_pipe* p, unsigned wake);
#endif

extern void oo_pipe_buf_clear_state(ci_netif* ni, struct oo_pipe* p);

#endif /* __ONLOAD_OO_PIPE_H__ */
