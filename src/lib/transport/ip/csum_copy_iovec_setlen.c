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

 /**************************************************************************\
 *//*! \file
 ** <L5_PRIVATE L5_SOURCE>
 ** \author  cjr/ctk
 **  \brief  Copy from iovec with Internet checksum.
 **   \date  2004/01/06
 **    \cop  (c) Level 5 Networks Limited.
 ** </L5_PRIVATE>
 *//*
 \**************************************************************************/

 /*! \cidoxg_lib_citools */

#include "ip_internal.h"


ci_inline int do_copy_from_user(void* to, const void* from, int n_bytes
                      CI_KERNEL_ARG(ci_addr_spc_t addr_spc))
{
# ifdef __KERNEL__
  if( addr_spc != CI_ADDR_SPC_KERNEL )
    return copy_from_user(to, from, n_bytes);
# endif
  memcpy(to, from, n_bytes);
  return 0;
}


int __ci_copy_iovec_to_pkt(ci_netif* ni, ci_ip_pkt_fmt* pkt,
                           ci_iovec_ptr* piov
                           CI_KERNEL_ARG(ci_addr_spc_t addr_spc))
{
  int n, total;
  char* dest;

  ci_assert(! ci_iovec_ptr_is_empty_proper(piov));
  ci_assert_gt(oo_offbuf_left(&pkt->buf), 0);

  dest = oo_offbuf_ptr(&pkt->buf);

  ci_assert_equal(pkt->n_buffers, 1);

  total = 0;
  while( 1 ) {
    n = oo_offbuf_left(&pkt->buf);
    n = CI_MIN(n, (int)CI_IOVEC_LEN(&piov->io));
    if(CI_UNLIKELY( do_copy_from_user(dest, CI_IOVEC_BASE(&piov->io), n
                                      CI_KERNEL_ARG(addr_spc)) ))
      return -EFAULT;

    /* TODO this isn't correct unless (n_buffers == 1) - it needs this
     * code to be updated to increment buf_len on current and
     * pay_len on first pkt in frag_next chain
     */
    pkt->buf_len += n;
    pkt->pay_len += n;

    total += n;
    ci_iovec_ptr_advance(piov, n);
    oo_offbuf_advance(&pkt->buf, n);

    /* We've either exhaused the source data (piov), the segment, or the
    ** space in the packet.  For latency critical apps, exhausting the
    ** source data is most likely, so we check for that first.
    */
    if( CI_IOVEC_LEN(&piov->io) == 0 ) {
      if( piov->iovlen == 0 )  goto done;
      --piov->iovlen;
      piov->io = *piov->iov++;
    }

    if( oo_offbuf_left(&pkt->buf) == 0 )  goto done;

    dest += n;
  }

done:
  return total;
}


#ifdef __KERNEL__
ssize_t
__ci_ip_copy_pkt_to_user(ci_netif* ni, ci_iovec* iov, ci_ip_pkt_fmt* pkt,
                         int peek_off)
{
  int len;

    len = CI_MIN(oo_offbuf_left(&pkt->buf) - peek_off, iov->iov_len);
    if( copy_to_user(CI_IOVEC_BASE(iov),
                     oo_offbuf_ptr(&pkt->buf) + peek_off, len) ) {
      ci_log("%s: faulted", __FUNCTION__);
      return -EFAULT;
    }
    oo_offbuf_advance(&pkt->buf, len);
    CI_IOVEC_BASE(iov) = (char *)CI_IOVEC_BASE(iov) + len;
    CI_IOVEC_LEN(iov) -= len;
    return len;
}

#else /* ifdef __KERNEL__ ... else */
ssize_t
__ci_ip_copy_pkt_to_user(ci_netif* ni, ci_iovec* iov,
                         ci_ip_pkt_fmt* pkt, int peek_off)
{
  int len;

  len = oo_offbuf_left(&pkt->buf) - peek_off;
  len = CI_MIN(len, (int) CI_IOVEC_LEN(iov));

  memcpy(CI_IOVEC_BASE(iov), oo_offbuf_ptr(&pkt->buf) + peek_off, len);

  oo_offbuf_advance(&pkt->buf, len);
  CI_IOVEC_BASE(iov) = (char *)CI_IOVEC_BASE(iov) + len;
  CI_IOVEC_LEN(iov) -= len;

  return len;
}
#endif  /* __KERNEL__ */


#ifdef __KERNEL__
/* Helper function for ci_ip_copy_pkt_from_piov() */
ci_inline size_t
ci_ip_follow_and_copy_page(ci_netif* ni, ci_ip_pkt_fmt* pkt,
                           ci_addr_spc_t addr_spc, char* src,
                           int offset, int len)
{
  struct page   *page;
  char          *dest;
  ci_iovec_ptr  piov;

  page = ci_follow_page(addr_spc, src);
  if (page == NULL)
    return 0;
  
  dest = ci_kmap_in_atomic(page);
  ci_assert(dest);
  
  len = CI_MIN(len, CI_PAGE_SIZE - offset);
  
  ci_iovec_ptr_init_buf(&piov, dest + offset, len);
  len = ci_copy_iovec_to_pkt(ni, pkt, &piov, CI_ADDR_SPC_KERNEL);
  
  ci_kunmap_in_atomic(page, dest);
  put_page(page);
  
  return len;
}


size_t
__ci_ip_copy_pkt_from_piov(
  ci_netif                        *ni,
  ci_ip_pkt_fmt                   *pkt,
  ci_iovec_ptr                    *piov,
  ci_addr_spc_t                   addr_spc)
{
  int                   total;

  if( addr_spc == CI_ADDR_SPC_KERNEL || addr_spc == CI_ADDR_SPC_CURRENT)
    return ci_copy_iovec_to_pkt(ni, pkt, piov, addr_spc);

  if( addr_spc == CI_ADDR_SPC_INVALID ) {
    /* ?? We want to know about this for now. */
    ci_log("%s: CI_ADDR_SPC_INVALID", __FUNCTION__);
    return 0;
  }

  total = 0;
  while( oo_offbuf_left(&pkt->buf) && ! ci_iovec_ptr_is_empty_proper(piov) ) {
    char *src = CI_IOVEC_BASE(&piov->io);
    int  offset = CI_PTR_OFFSET(src, CI_PAGE_SIZE);
    int  len = CI_IOVEC_LEN(&piov->io);

    len = ci_ip_follow_and_copy_page(ni, pkt, addr_spc, src, offset, len);
    if( len == 0 )
      break;
    total += len;
    ci_iovec_ptr_advance(piov, len);
  }

  return total;
}
                   


#else  /* ! __KERNEL__ */

size_t
__ci_ip_copy_pkt_from_piov(
  ci_netif                        *ni,
  ci_ip_pkt_fmt                   *pkt,
  ci_iovec_ptr                    *piov)
{
  /* ?? TODO: We should inline this in ip.h. */
  return ci_copy_iovec_to_pkt(ni, pkt, piov);
}

#endif  /* __KERNEL__ */
/*! \cidoxg_end */
