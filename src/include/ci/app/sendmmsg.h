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

/**************************************************************************\
*//*! \file
** <L5_PRIVATE L5_SOURCE>
** \author  kjm
**  \brief  Wrapper for sendmmsg to allow transparent usage on older systems
**   \date  2012/05/25
**    \cop  (c) Solarflare Communications Inc.
** </L5_PRIVATE>
*//*
\**************************************************************************/

/*! \cidoxg_include_ci_app */
#ifndef __CI_APP_SENDMMSG_H__

#include <dlfcn.h>

#include <linux/socket.h>

#include "libc_compat.h"

#if CI_LIBC_HAS_sendmmsg
# define SENDMMSG_AVAILABLE
#endif

#ifdef MSG_WAITFORONE
# define RECVMMSG_AVAILABLE
#endif


#if !defined(SENDMMSG_AVAILABLE) && !defined(RECVMMSG_AVAILABLE)
struct mmsghdr {
  struct msghdr  msg_hdr;
  unsigned       msg_len;
};
#endif


#ifdef __NR_sendmmsg
#ifdef __GNUC__
__attribute__((unused))
#endif
static int sc_sendmmsg(int fd, struct mmsghdr* mmsg, unsigned vlen, int flags)
{
  return syscall(__NR_sendmmsg, fd, mmsg, vlen, flags);
}
#endif


#ifndef SENDMMSG_AVAILABLE
#ifdef __GNUC__
__attribute__((unused))
#endif
static int sendmmsg(int fd, struct mmsghdr* mmsg, unsigned vlen, int flags)
{
  static int (*p_sendmmsg)(int, struct mmsghdr*, unsigned, int);
  if( p_sendmmsg == NULL ) {
    p_sendmmsg = (int (*)(int, struct mmsghdr*, unsigned, int))
      dlsym(RTLD_NEXT, "sendmmsg");
#ifdef __NR_sendmmsg
    if( p_sendmmsg == NULL )
      p_sendmmsg = sc_sendmmsg;
#endif
    if( p_sendmmsg == NULL ) {
      fprintf(stderr, "sendmmsg not linked and don't know value of"
              " __NR_sendmmsg\n");
      errno = ENOSYS;
      return -1;
    }
  }
  return p_sendmmsg(fd, mmsg, vlen, flags);
}
#endif

#endif /* __CI_APP_SENDMMSG_H__ */
/*! \cidoxg_end */
