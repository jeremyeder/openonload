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

#ifndef __ONLOAD_PKT_FILLER_H__
#define __ONLOAD_PKT_FILLER_H__


struct oo_pkt_filler {
  ci_ip_pkt_fmt* pkt;
  ci_ip_pkt_fmt* last_pkt;
  ci_ip_pkt_fmt* alloc_pkt;
  char*          buf_start;
  char*          buf_end;
};


/* Copy data from [piov] to the packet buffer in [pkt_filler].  Will
 * allocate additional packet buffers as necessary.  Advances [piov] to
 * reflect the amount of data consumed.
 *
 * Copies at most [bytes_to_copy].  [bytes_to_copy] must be <= the amount
 * of source data in [piov].
 *
 * For each packet that is filled completely (except the last), a segment
 * is initialised in [pf->pkt].
 *
 * Returns:
 *  - 0 on success
 *  - -EFAULT on mem fault (kernel only, SIGSEGV in userland)
 *  - -ERESTARTSYS if interrupted by a signal (kernel only)
 */
extern int oo_pkt_fill(ci_netif*, int* p_netif_locked,
                       struct oo_pkt_filler* pkt_filler,
                       ci_iovec_ptr* piov, int bytes_to_copy
                       CI_KERNEL_ARG(ci_addr_spc_t addr_spc)) CI_HF;


ci_inline void oo_pkt_filler_init(struct oo_pkt_filler* pf,
                                  ci_ip_pkt_fmt* pkt, void* buf_start) {
  pf->last_pkt = pf->pkt = pkt;
  pf->buf_start = buf_start;
  /* ?? or could this just be ((char*) pkt + 2048) ? */
  pf->buf_end = CI_PTR_ALIGN_FWD(PKT_START(pkt), 2048);
  /* ?? FIXME: should depend on runtime buffer size */
}


extern void oo_pkt_filler_free_unused_pkts(ci_netif* ni, int* p_netif_locked,
                                           struct oo_pkt_filler* pf) CI_HF;


ci_inline ci_ip_pkt_fmt* oo_pkt_filler_next_pkt(ci_netif* ni, 
                                                struct oo_pkt_filler* pf,
                                                int ni_locked)
{
  ci_ip_pkt_fmt* pkt = NULL;
  if( pf->alloc_pkt != NULL ) {
    pkt = pf->alloc_pkt;
    if( OO_PP_NOT_NULL(pkt->next) )
      pf->alloc_pkt = PKT_CHK_NML(ni, pkt->next, ni_locked);
    else
      pf->alloc_pkt = NULL;
  }
  return pkt;
}


ci_inline void oo_pkt_filler_add_pkt(struct oo_pkt_filler* pf,
                                     ci_ip_pkt_fmt* pkt)
{
  if( pf->alloc_pkt != NULL )
    pkt->next = OO_PKT_P(pf->alloc_pkt);
  else
    pkt->next = OO_PP_NULL;
  pf->alloc_pkt = pkt;
}


ci_inline int oo_pkt_fill_failed(int rc) {
#ifdef __KERNEL__
  ci_assert(rc == 0 || rc == -ERESTARTSYS || rc == -EFAULT);
  return rc < 0;
#else
  ci_assert_equal(rc, 0);
  return 0;
#endif
}


#endif  /* __ONLOAD_PKT_FILLER_H__ */
