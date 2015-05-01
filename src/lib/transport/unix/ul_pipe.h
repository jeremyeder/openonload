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

#ifndef __ONLOAD_PIPE_H__
#define __ONLOAD_PIPE_H__

#if !CI_CFG_USERSPACE_PIPE
#error "Do not include ul_pipe.h when pipe is not enabled"
#endif

#include "internal.h"

typedef struct {
  citp_fdinfo        fdinfo;
  struct oo_pipe*    pipe;
  ci_netif*          ni;
} citp_pipe_fdi;

#define fdi_to_pipe_fdi(_fdi) CI_CONTAINER(citp_pipe_fdi, fdinfo, (_fdi))

extern int citp_pipe_create(int fds[2], int flags);

extern int citp_splice_pipe_pipe(citp_pipe_fdi* in_pipe_fdi,
                                 citp_pipe_fdi* out_pipe_fdi, size_t rlen,
                                 int flags);
extern int citp_pipe_splice_write(citp_fdinfo* fdi, int alien_fd,
                                  loff_t* alien_off,
                                  size_t len, int flags,
                                  citp_lib_context_t* lib_context);
extern int citp_pipe_splice_read(citp_fdinfo* fdi, int alien_fd,
                                 loff_t* alien_off,
                                 size_t len, int flags,
                                 citp_lib_context_t* lib_context);

#endif  /* ul_pipe.h */
