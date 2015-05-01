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

#ifndef __UNIX_NONSOCK_H__
#define __UNIX_NONSOCK_H__
#include <ci/internal/transport_config_opt.h>
#include "internal.h"

/*************************************************************************
 ***************** Common non-socket handlers code ***********************
 *************************************************************************/

extern
int citp_passthrough_fcntl(citp_fdinfo *fdi, int cmd, long arg);
extern
int citp_passthrough_select(citp_fdinfo* fdinfo, int* n, int rd, int wr, int ex,
                            struct oo_ul_select_state*);
extern
int citp_passthrough_poll(citp_fdinfo* fdinfo, struct pollfd* pfd,
                          struct oo_ul_poll_state* ps);
extern
int citp_nonsock_bind(citp_fdinfo* fdinfo,
                      const struct sockaddr* sa, socklen_t sa_len);
extern
int citp_nonsock_listen(citp_fdinfo* fdinfo, int backlog);
extern
int citp_nonsock_accept(citp_fdinfo* fdinfo,
                        struct sockaddr* sa, socklen_t* p_sa_len, int flags,
                        citp_lib_context_t* lib_context);
extern
int citp_nonsock_connect(citp_fdinfo* fdinfo,
                         const struct sockaddr* sa, socklen_t sa_len,
                         citp_lib_context_t* lib_context);
extern
int citp_nonsock_shutdown(citp_fdinfo* fdinfo, int how);
extern
int citp_nonsock_getsockname(citp_fdinfo* fdinfo,
                             struct sockaddr* sa, socklen_t* p_sa_len);
extern
int citp_nonsock_getpeername(citp_fdinfo* fdinfo,
                             struct sockaddr* sa, socklen_t* p_sa_len);
extern
int citp_nonsock_getsockopt(citp_fdinfo* fdinfo, int level,
                            int optname, void* optval, socklen_t* optlen);
extern
int citp_nonsock_setsockopt(citp_fdinfo* fdinfo, int level, int optname,
                            const void* optval, socklen_t optlen);
extern
int citp_nonsock_recv(citp_fdinfo* fdinfo, struct msghdr* msg,
                          int flags);
extern
int citp_nonsock_send(citp_fdinfo* fdinfo, const struct msghdr* msg,
                          int flags);
#if CI_CFG_RECVMMSG
extern
int citp_nonsock_recvmmsg(citp_fdinfo* fdinfo, struct mmsghdr* msg,
                          unsigned vlen, int flags,
                          const struct timespec *timeout);
#endif
#if CI_CFG_SENDMMSG
extern
int citp_nonsock_sendmmsg(citp_fdinfo* fdinfo, struct mmsghdr* msg,
                          unsigned vlen, int flags);
#endif
extern
int citp_nonsock_zc_send(citp_fdinfo* fdi, struct onload_zc_mmsg* msg,
                         int flags);
extern
int citp_nonsock_zc_recv(citp_fdinfo* fdi,
                         struct onload_zc_recv_args* args);
extern
int citp_nonsock_recvmsg_kernel(citp_fdinfo* fdi, struct msghdr *msg,
                                int flags);
extern
int citp_nonsock_zc_recv_filter(citp_fdinfo* fdi,
                                onload_zc_recv_filter_callback filter,
                                void* cb_arg, int flags);
extern
int citp_nonsock_tmpl_alloc(citp_fdinfo* fdi, struct iovec* initial_msg,
                            int mlen, struct oo_msg_template** omt_pp,
                            unsigned flags);
extern
int citp_nonsock_tmpl_update(citp_fdinfo* fdi, struct oo_msg_template* omt,
                             struct onload_template_msg_update_iovec* updates,
                             int ulen, unsigned flags);
extern
int citp_nonsock_tmpl_abort(citp_fdinfo* fdi, struct oo_msg_template* omt);

#if CI_CFG_USERSPACE_EPOLL
extern
int citp_nonsock_ordered_data(citp_fdinfo* fdi, struct timespec* limit,
                              struct timespec* first_out, int* bytes_out);
#endif

#endif
